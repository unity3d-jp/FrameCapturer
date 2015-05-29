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
    void writeFrame(void *tex, int width, int height, fcETextureFormat fmt, const char *path);

private:
    void writeFrameTask(std::string &raw_frame, int width, int height, fcETextureFormat fmt, const std::string &path);

private:
    int m_magic; //  for debug
    fcExrConfig m_conf;
    std::vector<std::string> m_raw_frames;
    int m_frame;
#ifdef fcWithTBB
    std::atomic_int m_active_task_count;
    tbb::task_group m_tasks;
#endif
};


fcExrContext::fcExrContext(fcExrConfig &conf)
    : m_magic(fcE_ExrContext)
    , m_conf(conf)
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
void fcWriteExrFile(const char *path, char *data, int width, int height, int ch)
{
    Imf::PixelType pixel_type = fcTPixelType<T>::type;
    int tsize = sizeof(T);

    if (ch==4)
    {
        Imf::Header header(width, height);
        header.channels().insert("R", Imf::Channel(pixel_type));
        header.channels().insert("G", Imf::Channel(pixel_type));
        header.channels().insert("B", Imf::Channel(pixel_type));
        header.channels().insert("A", Imf::Channel(pixel_type));
        Imf::OutputFile fout(path, header);
        Imf::FrameBuffer frame_buffer;
        frame_buffer.insert("R", Imf::Slice(pixel_type, data + tsize * 0, tsize * 4, tsize * 4 * width));        frame_buffer.insert("G", Imf::Slice(pixel_type, data + tsize * 1, tsize * 4, tsize * 4 * width));        frame_buffer.insert("B", Imf::Slice(pixel_type, data + tsize * 2, tsize * 4, tsize * 4 * width));        frame_buffer.insert("A", Imf::Slice(pixel_type, data + tsize * 3, tsize * 4, tsize * 4 * width));        fout.setFrameBuffer(frame_buffer);
        fout.writePixels(height);
    }
    else if (ch == 2)
    {
        Imf::Header header(width, height);
        header.channels().insert("R", Imf::Channel(pixel_type));
        header.channels().insert("G", Imf::Channel(pixel_type));
        Imf::OutputFile fout(path, header);
        Imf::FrameBuffer frame_buffer;
        frame_buffer.insert("R", Imf::Slice(pixel_type, data + tsize * 0, tsize * 2, tsize * 2 * width));        frame_buffer.insert("G", Imf::Slice(pixel_type, data + tsize * 1, tsize * 2, tsize * 2 * width));        fout.setFrameBuffer(frame_buffer);
        fout.writePixels(height);
    }
    else if (ch == 1)
    {
        Imf::Header header(width, height);
        header.channels().insert("R", Imf::Channel(pixel_type));
        Imf::OutputFile fout(path, header);
        Imf::FrameBuffer frame_buffer;
        frame_buffer.insert("R", Imf::Slice(pixel_type, data, tsize, tsize * width));        fout.setFrameBuffer(frame_buffer);
        fout.writePixels(height);
    }
}


void fcExrContext::writeFrameTask(std::string &raw_frame, int width, int height, fcETextureFormat fmt, const std::string &path_)
{
    const char *path = path_.c_str();
    char *data = &raw_frame[0];
    switch (fmt)
    {
    case fcE_ARGBHalf:  fcWriteExrFile<half>(path, data, width, height, 4); break;
    case fcE_RGHalf:    fcWriteExrFile<half>(path, data, width, height, 2); break;
    case fcE_RHalf:     fcWriteExrFile<half>(path, data, width, height, 1); break;
    case fcE_ARGBFloat: fcWriteExrFile<float>(path, data, width, height, 4); break;
    case fcE_RGFloat:   fcWriteExrFile<float>(path, data, width, height, 2); break;
    case fcE_RFloat:    fcWriteExrFile<float>(path, data, width, height, 1); break;
    case fcE_ARGBInt:   fcWriteExrFile<uint>(path, data, width, height, 4); break;
    case fcE_RGInt:     fcWriteExrFile<uint>(path, data, width, height, 2); break;
    case fcE_RInt:      fcWriteExrFile<uint>(path, data, width, height, 1); break;
    }
}

void fcExrContext::writeFrame(void *tex, int width, int height, fcETextureFormat fmt, const char *_path)
{
#ifdef fcWithTBB
    if (m_active_task_count >= m_conf.max_active_tasks)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (m_active_task_count >= m_conf.max_active_tasks)
        {
            m_tasks.wait();
        }
    }
#endif
    int frame = m_frame++;
    std::string& raw_frame = m_raw_frames[frame % m_conf.max_active_tasks];
    raw_frame.resize(width*height * fcGetPixelSize(fmt));

    // フレームバッファの内容取得
    fcGetGraphicsDevice()->copyTextureData(&raw_frame[0], raw_frame.size(), tex, width, height, fmt);

    // exr 書き出しタスクを kick
    std::string path = _path;
#ifdef fcWithTBB
    ++m_active_task_count;
    m_tasks.run([this, &raw_frame, width, height, fmt, path](){
        writeFrameTask(raw_frame, width, height, fmt, path);
        --m_active_task_count;
    });
#else
    writeFrameTask(raw_frame, fmt, path);
#endif
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

fcCLinkage fcExport void fcExrWriteFile(fcExrContext *ctx, void *tex, int width, int height, fcETextureFormat fmt, const char *path)
{
    fcCheckContext(ctx);
    ctx->writeFrame(tex, width, height, fmt, path);
}

#endif // fcSupportEXR
