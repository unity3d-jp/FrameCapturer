#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"

#ifdef fcSupportEXR
#include <half.h>
#include <ImfRgbaFile.h>
#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfArray.h>
#pragma comment(lib, "Half.lib")
#pragma comment(lib, "Iex-2_2.lib")
#pragma comment(lib, "IexMath-2_2.lib")
#pragma comment(lib, "IlmThread-2_2.lib")
#pragma comment(lib, "IlmImf-2_2.lib")
#pragma comment(lib, "zlibstatic.lib")


class fcExrContext
{
public:
    fcExrContext(fcExrConfig &conf);
    ~fcExrContext();
    bool writeFrame(const char *path, void *tex, int width, int height, fcETextureFormat fmt, int mask);

private:
    void writeFrameTask(const std::string &path, std::string &raw_frame, int width, int height, fcETextureFormat fmt, int mask);

    struct WorkData
    {
        std::string buffer;
        std::atomic_int refcount;
    };

private:
    int m_magic; //  for debug
    fcExrConfig m_conf;
    std::vector<WorkData> m_raw_frames;
    int m_frame;
#ifdef fcWithTBB
    tbb::task_group m_tasks;
#endif
};


fcExrContext::fcExrContext(fcExrConfig &conf)
    : m_magic(fcE_ExrContext)
    , m_conf(conf)
    , m_frame(0)
{
    m_raw_frames.resize(m_conf.max_active_tasks);
}

fcExrContext::~fcExrContext()
{
#ifdef fcWithTBB
    m_tasks.wait();
#endif
}


typedef unsigned int uint;
template<class T> struct fcTPixelType;
template<> struct fcTPixelType < half > { static const Imf::PixelType type = Imf::HALF; };
template<> struct fcTPixelType < float >{ static const Imf::PixelType type = Imf::FLOAT; };
template<> struct fcTPixelType < uint > { static const Imf::PixelType type = Imf::UINT; };

template<class T>
void fcWriteExrFile(const char *path, char *data, int width, int height, int ch, int mask)
{
    Imf::PixelType pixel_type = fcTPixelType<T>::type;
    int tsize = sizeof(T);
    int psize = tsize * ch;
    const char names[4][2] = { "R", "G", "B", "A", };

    if (ch==4)
    {
        Imf::Header header(width, height);
        Imf::FrameBuffer frame_buffer;
        for (int i = 0; i < 4; ++i) {
            if ((mask & (1 << i)) != 0) {
                header.channels().insert(names[i], Imf::Channel(pixel_type));
                frame_buffer.insert(names[i], Imf::Slice(pixel_type, data + tsize * i, psize, psize * width));
            }
        }
        Imf::OutputFile fout(path, header);
        fout.setFrameBuffer(frame_buffer);
        fout.writePixels(height);
    }
    else if (ch == 2)
    {
        Imf::Header header(width, height);
        Imf::FrameBuffer frame_buffer;
        for (int i = 0; i < 2; ++i) {
            if ((mask & (1 << i)) != 0) {
                header.channels().insert(names[i], Imf::Channel(pixel_type));
                frame_buffer.insert(names[i], Imf::Slice(pixel_type, data + tsize * i, psize, psize * width));
            }
        }
        Imf::OutputFile fout(path, header);
        fout.setFrameBuffer(frame_buffer);
        fout.writePixels(height);
    }
    else if (ch == 1)
    {
        Imf::Header header(width, height);
        Imf::FrameBuffer frame_buffer;
        for (int i = 0; i < 1; ++i) {
            if ((mask & (1 << i)) != 0) {
                header.channels().insert(names[i], Imf::Channel(pixel_type));
                frame_buffer.insert(names[i], Imf::Slice(pixel_type, data, psize, psize * width));
            }
        }
        Imf::OutputFile fout(path, header);
        fout.setFrameBuffer(frame_buffer);
        fout.writePixels(height);
    }
}


void fcExrContext::writeFrameTask(const std::string &path_, std::string &raw_frame, int width, int height, fcETextureFormat fmt, int mask)
{
    const char *path = path_.c_str();
    char *data = &raw_frame[0];
    switch (fmt)
    {
    case fcE_ARGBHalf:  fcWriteExrFile<half>(path, data, width, height, 4, mask); break;
    case fcE_RGHalf:    fcWriteExrFile<half>(path, data, width, height, 2, mask); break;
    case fcE_RHalf:     fcWriteExrFile<half>(path, data, width, height, 1, mask); break;
    case fcE_ARGBFloat: fcWriteExrFile<float>(path, data, width, height, 4, mask); break;
    case fcE_RGFloat:   fcWriteExrFile<float>(path, data, width, height, 2, mask); break;
    case fcE_RFloat:    fcWriteExrFile<float>(path, data, width, height, 1, mask); break;
    case fcE_ARGBInt:   fcWriteExrFile<uint>(path, data, width, height, 4, mask); break;
    case fcE_RGInt:     fcWriteExrFile<uint>(path, data, width, height, 2, mask); break;
    case fcE_RInt:      fcWriteExrFile<uint>(path, data, width, height, 1, mask); break;
    }
}

bool fcExrContext::writeFrame(const char *path_, void *tex, int width, int height, fcETextureFormat fmt, int mask)
{
    WorkData *wd = nullptr;

    // フレームバッファの内容取得
    if (tex == nullptr)
    {
        // tex が null の場合、前回取得した結果を使い回す。 (exr 書き出しの場合わりとよくあるケース)
        wd = &m_raw_frames[(m_frame - 1) % m_conf.max_active_tasks];
    }
    else
    {
        wd = &m_raw_frames[m_frame % m_conf.max_active_tasks];
#ifdef fcWithTBB
        if (wd->refcount > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (wd->refcount > 0) { m_tasks.wait(); }
        }
#endif
        wd->buffer.resize(width * height * fcGetPixelSize(fmt));
        if (!fcGetGraphicsDevice()->copyTextureData(&wd->buffer[0], wd->buffer.size(), tex, width, height, fmt))
        {
            return false;
        }
        ++m_frame;
    }

    // exr 書き出しタスクを kick
    std::string path = path_;
#ifdef fcWithTBB
    ++wd->refcount;
    m_tasks.run([this, path, wd, width, height, fmt, mask](){
        writeFrameTask(path, wd->buffer, width, height, fmt, mask);
        --wd->refcount;
    });
#else
    writeFrameTask(path, wd->buffer, width, height, fmt, mask);
#endif
    return true;
}



#ifdef fcDebug
#define fcCheckContext(v) if(v==nullptr || *(int*)v!=fcE_ExrContext) { fcBreak(); }
#else  // fcDebug
#define fcCheckContext(v) 
#endif // fcDebug

fcCLinkage fcExport fcExrContext* fcExrCreateContext(fcExrConfig *conf)
{
    return new fcExrContext(*conf);
}

fcCLinkage fcExport void fcExrDestroyContext(fcExrContext *ctx)
{
    fcCheckContext(ctx);
    delete ctx;
}

fcCLinkage fcExport bool fcExrWriteFile(fcExrContext *ctx, const char *path, void *tex, int width, int height, fcETextureFormat fmt, int mask)
{
    fcCheckContext(ctx);
    return ctx->writeFrame(path, tex, width, height, fmt, mask);
}

#endif // fcSupportEXR
