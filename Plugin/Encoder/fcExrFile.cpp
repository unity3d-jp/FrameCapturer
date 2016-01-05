#include "pch.h"
#include "FrameCapturer.h"

#ifdef fcSupportEXR
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
    std::list<std::string> raw_frames;
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
    fcExrContext(fcExrConfig &conf, fcIGraphicsDevice *dev);
    ~fcExrContext();
    void release() override;
    bool beginFrame(const char *path, int width, int height) override;
    bool addLayer(void *tex, fcETextureFormat fmt, int channel, const char *name, bool flipY, bool asPixels) override;
    bool endFrame() override;

private:
    void endFrameTask(fcExrFrameData *exr);

private:
    fcEMagic m_magic; //  for debug
    fcExrConfig m_conf;
    fcIGraphicsDevice *m_dev;
    fcExrFrameData *m_exr;
    fcTaskGroup m_tasks;
    std::atomic<int> m_active_task_count;

    void *m_tex_prev;
};


fcExrContext::fcExrContext(fcExrConfig &conf, fcIGraphicsDevice *dev)
    : m_magic(fcE_ExrContext)
    , m_conf(conf)
    , m_dev(dev)
    , m_exr(nullptr)
    , m_active_task_count(0)
    , m_tex_prev(nullptr)
{
}

fcExrContext::~fcExrContext()
{
    m_tasks.wait();
    m_magic = fcE_Deleted;
}



void fcExrContext::release()
{
    delete this;
}

bool fcExrContext::beginFrame(const char *path, int width, int height)
{
    if (m_exr != nullptr) { return false; } // beginFrame() されたまま endFrame() されてない

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

bool fcExrContext::addLayer(void *tex, fcETextureFormat fmt, int channel, const char *name, bool flipY, bool asPixels)
{
    std::string *raw_frame = nullptr;

    // フレームバッファの内容取得
    if (tex == m_tex_prev)
    {
        // 前回取得した結果を使い回す
        raw_frame = &m_exr->raw_frames.back();
    }
    else
    {
        m_tex_prev = tex;
        m_exr->raw_frames.push_back(std::string());
        raw_frame = &m_exr->raw_frames.back();
        raw_frame->resize(m_exr->width * m_exr->height * fcGetPixelSize(fmt));
        if (!m_dev->readTexture(&(*raw_frame)[0], raw_frame->size(), tex, m_exr->width, m_exr->height, fmt))
        {
            m_exr->raw_frames.pop_back();
            return false;
        }
    }

    {
        char *raw_data = &(*raw_frame)[0];
        Imf::PixelType pixel_type = Imf::HALF;
        int channels = 0;
        int tsize = 0;
        switch (fmt)
        {
        case fcE_ARGBHalf:  pixel_type = Imf::HALF; channels = 4; tsize = 2; break;
        case fcE_RGHalf:    pixel_type = Imf::HALF; channels = 2; tsize = 2; break;
        case fcE_RHalf:     pixel_type = Imf::HALF; channels = 1; tsize = 2; break;
        case fcE_ARGBFloat: pixel_type = Imf::FLOAT; channels = 4; tsize = 4; break;
        case fcE_RGFloat:   pixel_type = Imf::FLOAT; channels = 2; tsize = 4; break;
        case fcE_RFloat:    pixel_type = Imf::FLOAT; channels = 1; tsize = 4; break;
        case fcE_ARGBInt:   pixel_type = Imf::UINT; channels = 4; tsize = 4; break;
        case fcE_RGInt:     pixel_type = Imf::UINT; channels = 2; tsize = 4; break;
        case fcE_RInt:      pixel_type = Imf::UINT; channels = 1; tsize = 4; break;
        default:
        {
            m_exr->raw_frames.pop_back();
            return false;
        }
        }
        int psize = tsize * channels;

        m_exr->header.channels().insert(name, Imf::Channel(pixel_type));
        m_exr->frame_buffer.insert(name, Imf::Slice(pixel_type, raw_data + (tsize * channel), psize, psize * m_exr->width));
    }
    return true;
}

bool fcExrContext::endFrame()
{
    if (m_exr == nullptr) { return false; } // beginFrame() されてない
    m_tex_prev = nullptr;

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
    Imf::OutputFile fout(exr->path.c_str(), exr->header);
    fout.setFrameBuffer(exr->frame_buffer);
    fout.writePixels(exr->height);
    delete exr;
}


fcCLinkage fcExport fcIExrContext* fcExrCreateContextImpl(fcExrConfig &conf, fcIGraphicsDevice *dev)
{
    return new fcExrContext(conf, dev);
}
#endif // fcSupportEXR
