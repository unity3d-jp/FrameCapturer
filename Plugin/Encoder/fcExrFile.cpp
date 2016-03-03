#include "pch.h"

#include <half.h>
#include <ImfRgbaFile.h>
#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfArray.h>
#include "fcFoundation.h"
#include "fcThreadPool.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcExrFile.h"

#if defined(fcWindows) && !defined(fcNoAutoLink)
    #pragma comment(lib, "Half.lib")
    #pragma comment(lib, "Iex-2_2.lib")
    #pragma comment(lib, "IexMath-2_2.lib")
    #pragma comment(lib, "IlmThread-2_2.lib")
    #pragma comment(lib, "IlmImf-2_2.lib")
    #pragma comment(lib, "zlibstatic.lib")
#endif



struct fcExrFrameData
{
    std::string path;
    int width, height;
    std::list<Buffer> pixels;
    Imf::Header header;
    Imf::FrameBuffer frame_buffer;

    fcExrFrameData(const char *p, int w, int h)
        : path(p), width(w), height(h), header(w, h)
    {
        header.compression() = Imf::ZIPS_COMPRESSION;
    }
};

class fcExrContext : public fcIExrContext
{
public:
    fcExrContext(const fcExrConfig& conf, fcIGraphicsDevice *dev);
    ~fcExrContext();
    void release() override;
    bool beginFrame(const char *path, int width, int height) override;
    bool addLayerTexture(void *tex, fcPixelFormat fmt, int channel, const char *name, bool flipY) override;
    bool addLayerPixels(const void *pixels, fcPixelFormat fmt, int channel, const char *name, bool flipY) override;
    bool endFrame() override;

private:
    bool addLayerImpl(char *pixels, fcPixelFormat fmt, int channel, const char *name);
    void endFrameTask(fcExrFrameData *exr);

private:
    fcExrConfig m_conf;
    fcIGraphicsDevice *m_dev;
    fcExrFrameData *m_exr;
    fcTaskGroup m_tasks;
    std::atomic_int m_active_task_count;

    const void *m_frame_prev;
    Buffer *m_src_prev;
    fcPixelFormat m_fmt_prev;
};


fcExrContext::fcExrContext(const fcExrConfig& conf, fcIGraphicsDevice *dev)
    : m_conf()
    , m_dev(dev)
    , m_exr(nullptr)
    , m_active_task_count(0)
    , m_frame_prev(nullptr)
    , m_src_prev(nullptr)
    , m_fmt_prev()
{
    m_conf = conf;
    if (m_conf.max_active_tasks <= 0) {
        m_conf.max_active_tasks = std::thread::hardware_concurrency();
    }
}

fcExrContext::~fcExrContext()
{
    m_tasks.wait();
}


void fcExrContext::release()
{
    delete this;
}

bool fcExrContext::beginFrame(const char *path, int width, int height)
{
    if (m_exr != nullptr) {
        fcDebugLog("fcExrContext::beginFrame(): beginFrame() is already called. maybe you forgot to call endFrame().");
        return false;
    }

    // 実行中のタスクの数が上限に達している場合適当に待つ
    if (m_active_task_count >= m_conf.max_active_tasks)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (m_active_task_count >= m_conf.max_active_tasks)
        {
            m_tasks.wait();
        }
    }

    m_exr = new fcExrFrameData(path, width, height);
    return true;
}


// pitch: width * pixel size
static void fcImageFlipY(void *image_, int width, int height, int pitch)
{
    Buffer buf_((size_t)pitch);
    char *image = (char*)image_;
    char *buf = &buf_[0];

    for (int y = 0; y < height / 2; ++y) {
        int iy = height - y - 1;
        memcpy(buf, image + (pitch*y), pitch);
        memcpy(image + (pitch*y), image + (pitch*iy), pitch);
        memcpy(image + (pitch*iy), buf, pitch);
    }
}

bool fcExrContext::addLayerTexture(void *tex, fcPixelFormat fmt, int channel, const char *name, bool flipY)
{
    if (m_exr == nullptr) {
        fcDebugLog("fcExrContext::addLayerTexture(): maybe beginFrame() is not called.");
        return false;
    }

    Buffer *raw_frame = nullptr;

    if (tex == m_frame_prev)
    {
        raw_frame = m_src_prev;
        fmt = m_fmt_prev;
    }
    else
    {
        m_frame_prev = tex;

        m_exr->pixels.push_back(Buffer());
        raw_frame = &m_exr->pixels.back();
        raw_frame->resize(m_exr->width * m_exr->height * fcGetPixelSize(fmt));

        // get frame buffer
        if (!m_dev->readTexture(&(*raw_frame)[0], raw_frame->size(), tex, m_exr->width, m_exr->height, fmt))
        {
            m_exr->pixels.pop_back();
            return false;
        }
        if (flipY) {
            fcImageFlipY(&(*raw_frame)[0], m_exr->width, m_exr->height, m_exr->width * fcGetPixelSize(fmt));
        }
        m_src_prev = raw_frame;

        // convert pixel format if it is not supported by exr
        if ((fmt & fcPixelFormat_TypeMask) == fcPixelFormat_Type_u8) {
            m_exr->pixels.push_back(std::vector<char>());
            auto *buf = &m_exr->pixels.back();

            int channels = fmt & fcPixelFormat_ChannelMask;
            auto src_fmt = fmt;
            fmt = fcPixelFormat(fcPixelFormat_Type_f16 | channels);
            raw_frame->resize(m_exr->width * m_exr->height * fcGetPixelSize(fmt));
            fcConvertPixelFormat(&(*buf)[0], fmt, &(*raw_frame)[0], src_fmt, m_exr->width * m_exr->height);

            m_src_prev = raw_frame = buf;
        }

        m_fmt_prev = fmt;
    }

    return addLayerImpl(&(*raw_frame)[0], fmt, channel, name);
}

bool fcExrContext::addLayerPixels(const void *pixels, fcPixelFormat fmt, int channel, const char *name, bool flipY)
{
    if (m_exr == nullptr) {
        fcDebugLog("fcExrContext::addLayerPixels(): maybe beginFrame() is not called.");
        return false;
    }

    Buffer *raw_frame = nullptr;

    if (pixels == m_frame_prev)
    {
        raw_frame = m_src_prev;
        fmt = m_fmt_prev;
    }
    else
    {
        m_frame_prev = pixels;

        m_exr->pixels.push_back(std::vector<char>());
        raw_frame = &m_exr->pixels.back();

        // convert pixel format if it is not supported by exr
        if ((fmt & fcPixelFormat_TypeMask) == fcPixelFormat_Type_u8) {
            int channels = fmt & fcPixelFormat_ChannelMask;
            auto src_fmt = fmt;
            fmt = fcPixelFormat(fcPixelFormat_Type_f16 | channels);
            raw_frame->resize(m_exr->width * m_exr->height * (2 * channels));
            fcConvertPixelFormat(&(*raw_frame)[0], fmt, pixels, src_fmt, m_exr->width * m_exr->height);
        }
        else {
            raw_frame->resize(m_exr->width * m_exr->height * fcGetPixelSize(fmt));
            memcpy(&(*raw_frame)[0], pixels, raw_frame->size());
        }
        if (flipY) {
            fcImageFlipY(&(*raw_frame)[0], m_exr->width, m_exr->height, m_exr->width * fcGetPixelSize(fmt));
        }

        m_src_prev = raw_frame;
        m_fmt_prev = fmt;
    }

    return addLayerImpl(&(*raw_frame)[0], fmt, channel, name);
}

bool fcExrContext::addLayerImpl(char *pixels, fcPixelFormat fmt, int channel, const char *name)
{
    Imf::PixelType pixel_type = Imf::HALF;
    int channels = fmt & fcPixelFormat_ChannelMask;
    int tsize = 0;
    switch (fmt & fcPixelFormat_TypeMask)
    {
    case fcPixelFormat_Type_f16:
        pixel_type = Imf::HALF;
        tsize = 2;
        break;
    case fcPixelFormat_Type_f32:
        pixel_type = Imf::FLOAT;
        tsize = 4;
        break;
    case fcPixelFormat_Type_i32:
        pixel_type = Imf::UINT;
        tsize = 4;
        break;
    default:
        fcDebugLog("fcExrContext::addLayerPixels(): this pixel format is not supported");
        return false;
    }
    int psize = tsize * channels;

    m_exr->header.channels().insert(name, Imf::Channel(pixel_type));
    m_exr->frame_buffer.insert(name, Imf::Slice(pixel_type, pixels + (tsize * channel), psize, psize * m_exr->width));
    return true;
}


bool fcExrContext::endFrame()
{
    if (m_exr == nullptr) {
        fcDebugLog("fcExrContext::endFrame(): maybe beginFrame() is not called.");
        return false;
    }

    m_frame_prev = nullptr;

    fcExrFrameData *exr = m_exr;
    m_exr = nullptr;
    ++m_active_task_count;
    m_tasks.run([this, exr](){
        endFrameTask(exr);
        --m_active_task_count;
    });
    return true;
}

void fcExrContext::endFrameTask(fcExrFrameData *exr)
{
    try {
        Imf::OutputFile fout(exr->path.c_str(), exr->header);
        fout.setFrameBuffer(exr->frame_buffer);
        fout.writePixels(exr->height);
        delete exr;
    }
    catch (std::string &e) {
        fcDebugLog(e.c_str());
    }
}


fcCLinkage fcExport fcIExrContext* fcExrCreateContextImpl(const fcExrConfig *conf, fcIGraphicsDevice *dev)
{
    fcExrConfig default_cont;
    if (conf == nullptr) { conf = &default_cont; }
    return new fcExrContext(*conf, dev);
}
