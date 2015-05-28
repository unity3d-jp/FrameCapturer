#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"
#include "jo_gif.cpp"

class fcGifContext
{
public:
    fcGifContext(const char *path, int width, int height);
    ~fcGifContext();
    void addFrame(void *tex);

private:
    int m_magic;
    int m_width;
    int m_height;
    std::string m_path;
    std::string m_buf;
    jo_gif_t m_gif;
    void *m_tmp_tex;
};


fcGifContext::fcGifContext(const char *path, int width, int height)
    : m_magic(fcE_GifContext)
    , m_width(width)
    , m_height(height)
    , m_path(path)
    , m_tmp_tex(nullptr)
{
    m_gif = jo_gif_start(path, width, height, 0, 255);
    m_buf.resize(width*height*4);
}

fcGifContext::~fcGifContext()
{
    jo_gif_end(&m_gif);
    fcGetGraphicsDevice()->releaseTmpTexture(m_tmp_tex);
}

void fcGifContext::addFrame(void *tex)
{
    bool palette = false;
    if (m_tmp_tex==nullptr)
    {
        m_tmp_tex = fcGetGraphicsDevice()->createTmpTexture(m_width, m_height, fcE_ARGB32);
    }
    fcGetGraphicsDevice()->copyTextureData(&m_buf[0], tex, m_tmp_tex, m_width, m_height, fcE_ARGB32);
    jo_gif_frame(&m_gif, (unsigned char*)&m_buf[0], 3, palette);
}


#ifdef fcDebug
#define fcCheckContext(v) if(v==nullptr || *(int*)v!=fcE_GifContext) { fcBreak(); }
#else  // fcDebug
#define fcCheckContext(v) 
#endif // fcDebug


fcCLinkage fcExport fcGifContext* fcGifCreateFile(const char *path, int width, int height)
{
    return new fcGifContext(path, width, height);
}

fcCLinkage fcExport void fcGifCloseFile(fcGifContext *ctx)
{
    fcCheckContext(ctx);
    delete ctx;
}

fcCLinkage fcExport void fcGifAddFrame(fcGifContext *ctx, void *tex)
{
    fcCheckContext(ctx);
    ctx->addFrame(tex);
}

fcCLinkage fcExport void fcGifBeginAddFrame(fcGifContext *ctx, void *tex)
{
    fcCheckContext(ctx);
}

fcCLinkage fcExport void fcGifEndAddFrame(fcGifContext *ctx)
{
    fcCheckContext(ctx);
}
