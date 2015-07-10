#include "pch.h"
#include "FrameCapturer.h"
#include "fcThreadPool.h"
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

#if defined(fcWindows) && !defined(fcNoAutoLink)
#pragma comment(lib, "Half.lib")
#pragma comment(lib, "Iex-2_2.lib")
#pragma comment(lib, "IexMath-2_2.lib")
#pragma comment(lib, "IlmThread-2_2.lib")
#pragma comment(lib, "IlmImf-2_2.lib")
#pragma comment(lib, "zlibstatic.lib")
#endif

#include "fcExrFile.h"



struct fcExrFrameData
{
    std::string path;
    int width, height;
    std::list<void*> rawFrames;
    Imf::Header header;
    Imf::FrameBuffer frameBuffer;

    fcExrFrameData(const char *p, int w, int h)
        : path(p), width(w), height(h), header(w, h)
    {
        header.compression() = Imf::ZIPS_COMPRESSION;
    }

    ~fcExrFrameData()
    {
        for (std::list<void*>::iterator it=rawFrames.begin(); it!=rawFrames.end(); ++it)
        {
            free(*it);
        }
        rawFrames.clear();
    }
};

class fcExrContext : public fcIExrContext
{
public:
    fcExrContext(fcExrConfig &conf, fcIGraphicsDevice *dev);
    ~fcExrContext();

    void release() override;
    bool beginFrame(const char *path, int width, int height) override;
    bool addLayer(void *tex, fcETextureFormat fmt, int channel, const char *name, bool flipY) override;
    bool endFrame() override;

private:
    fcEMagic m_magic; //  for debug
    fcExrConfig m_conf;
    fcIGraphicsDevice *m_dev;
    fcExrFrameData *m_exr;

    void *m_lastTex;
};


fcIExrContext* fcCreateExrContext(fcExrConfig &conf, fcIGraphicsDevice *dev)
{
    return new fcExrContext(conf, dev);
}


fcExrContext::fcExrContext(fcExrConfig &conf, fcIGraphicsDevice *dev)
    : m_magic(fcE_ExrContext)
    , m_conf(conf)
    , m_dev(dev)
    , m_exr(nullptr)
    , m_lastTex(nullptr)
{
}

fcExrContext::~fcExrContext()
{
    m_magic = fcE_Deleted;

    if (m_exr != nullptr)
    {
        delete m_exr;
    }
}



void fcExrContext::release()
{
    delete this;
}

bool fcExrContext::beginFrame(const char *path, int width, int height)
{
    if (m_exr != nullptr)
    {
        // beginFrame() されたまま endFrame() されてない
        return false;
    } 

    m_exr = new fcExrFrameData(path, width, height);

    return true;
}

bool fcExrContext::addLayer(void *tex, fcETextureFormat fmt, int channel, const char *name, bool flipY)
{
    Imf::PixelType pixelType = Imf::HALF;
    int tsize = 2;
    int psize = 0;
    int channels = 0;
    void *rawFrame = nullptr;

    switch (fmt)
    {
    case fcE_ARGBFloat:
        pixelType = Imf::FLOAT;
        tsize = 4;
    case fcE_ARGBHalf:
        channels = 4;
        break;
    case fcE_RGFloat:
        pixelType = Imf::FLOAT;
        tsize = 4;
    case fcE_RGHalf:
        channels = 2;
        break;
    case fcE_RFloat:
        pixelType = Imf::FLOAT;
        tsize = 4;
    case fcE_RHalf:
        channels = 1;
        break;
    default:
        return false;
    }

    psize = tsize * channels;

    // フレームバッファの内容取得
    if (tex == m_lastTex)
    {
        // 前回取得した結果を使い回す
        rawFrame = m_exr->rawFrames.back();
    }
    else
    {
        m_lastTex = tex;

        size_t bufSize = m_exr->width * m_exr->height * psize;
        
        rawFrame = malloc(bufSize);
        
        m_exr->rawFrames.push_back(rawFrame);

        if (!m_dev->readTexture(rawFrame, bufSize, tex, m_exr->width, m_exr->height, fmt))
        {
            free(rawFrame);
            m_exr->rawFrames.pop_back();
            return false;
        }
        else if (flipY)
        {
            size_t lineSize = m_exr->width * psize;
            char *tmp = (char*) malloc(lineSize);
            int hh = m_exr->height / 2;
            char *line0 = (char*)rawFrame;
            char *line1 = line0 + (m_exr->height - 1) * lineSize;

            for (int h=0; h<hh; ++h)
            {
                memcpy(tmp, line0, lineSize);
                memcpy(line0, line1, lineSize);
                memcpy(line1, tmp, lineSize);
                line0 += lineSize;
                line1 -= lineSize;
            }

            free(tmp);
        }
    }

    m_exr->header.channels().insert(name, Imf::Channel(pixelType));
    m_exr->frameBuffer.insert(name, Imf::Slice(pixelType, (char*)rawFrame + (tsize * channel), psize, psize * m_exr->width));
    
    return true;
}

bool fcExrContext::endFrame()
{
    if (m_exr == nullptr)
    {
        // beginFrame() されてない
        return false;
    }

    Imf::OutputFile fout(m_exr->path.c_str(), m_exr->header);
    
    fout.setFrameBuffer(m_exr->frameBuffer);
    fout.writePixels(m_exr->height);

    delete m_exr;
    
    m_lastTex = nullptr;
    m_exr = nullptr;

    return true;
}

#endif // fcSupportEXR

