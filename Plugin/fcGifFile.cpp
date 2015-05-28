#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"
#include "jo_gif.cpp"

class fcGifContext
{
public:
    struct WorkData
    {
        std::string gif;
        std::string raw;
    };
public:
    fcGifContext(const char *path, fcGifConfig *conf);
    ~fcGifContext();
    void addFrameTask(int i);
    void addFrame(void *tex);

private:
    int m_magic; //  for debug
    fcGifConfig m_conf;
    std::string m_path;
    std::vector<WorkData> m_work;
    tbb::task_group m_tasks;
    jo_gif_t m_gif;
    void *m_tmp_tex;
    volatile int m_frame_count;
    volatile int m_active_task_count;
};


fcGifContext::fcGifContext(const char *path, fcGifConfig *conf)
    : m_magic(fcE_GifContext)
    , m_conf(*conf)
    , m_path(path)
    , m_tmp_tex(nullptr)
    , m_frame_count(0)
    , m_active_task_count(0)
{
    m_gif = jo_gif_start(path, m_conf.width, m_conf.height, 0, 255);
    m_work.resize(m_conf.max_active_tasks);
    for (auto& wd : m_work)
    {
        wd.raw.resize(m_conf.width * m_conf.height * 4);
    }
}

fcGifContext::~fcGifContext()
{
    m_tasks.wait();
    jo_gif_end(&m_gif);
    fcGetGraphicsDevice()->releaseTmpTexture(m_tmp_tex);
}

void fcGifContext::addFrameTask(int f)
{
    int w = f % m_conf.max_active_tasks;
    bool update_palette = m_conf.keyframe != 0 && f%m_conf.keyframe == 0;
    jo_gif_frame(&m_gif, (unsigned char*)&m_work[w].raw[0], 3, update_palette);
    fcAtomicDecrement(&m_active_task_count);
}

void fcGifContext::addFrame(void *tex)
{
    if (m_tmp_tex==nullptr)
    {
        m_tmp_tex = fcGetGraphicsDevice()->createTmpTexture(m_conf.width, m_conf.height, fcE_ARGB32);
    }


    //if (m_active_task_count >= m_conf.max_active_tasks)
    {
        m_tasks.wait();
    }
    fcAtomicIncrement(&m_active_task_count);
    int f = ++m_frame_count;
    int w = f % m_conf.max_active_tasks;

    // get raw framebuffer
    fcGetGraphicsDevice()->copyTextureData(&m_work[w].raw[0], tex, m_tmp_tex, m_conf.width, m_conf.height, fcE_ARGB32);

    // compress asynchronously
    m_tasks.run([=](){ addFrameTask(f % m_conf.max_active_tasks); });
}


#ifdef fcDebug
#define fcCheckContext(v) if(v==nullptr || *(int*)v!=fcE_GifContext) { fcBreak(); }
#else  // fcDebug
#define fcCheckContext(v) 
#endif // fcDebug


fcCLinkage fcExport fcGifContext* fcGifCreateFile(const char *path, fcGifConfig *conf)
{
    return new fcGifContext(path, conf);
}

fcCLinkage fcExport void fcGifCloseFile(fcGifContext *ctx)
{
    fcCheckContext(ctx);
    delete ctx;
}

fcCLinkage fcExport void fcGifWriteFrame(fcGifContext *ctx, void *tex)
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
