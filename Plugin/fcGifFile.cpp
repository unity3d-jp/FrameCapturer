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
    void scrape();
    void addFrameTask(int frame, bool local_palette, std::string &buf);
    void addFrame(void *tex);

private:
    int m_magic; //  for debug
    fcGifConfig m_conf;
    std::string m_path;
    std::vector<WorkData> m_work;
    std::list<std::string> m_frames;
    tbb::task_group m_tasks;
    jo_gif_t m_gif;
    void *m_tmp_tex;
    int m_frame;
    volatile int m_active_task_count;
};


fcGifContext::fcGifContext(const char *path, fcGifConfig *conf)
    : m_magic(fcE_GifContext)
    , m_conf(*conf)
    , m_path(path)
    , m_tmp_tex(nullptr)
    , m_frame(0)
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
    scrape();
    for (auto &buf : m_frames) {
        fwrite(buf.c_str(), buf.size(), 1, m_gif.fp);
    }
    jo_gif_end(&m_gif);
    fcGetGraphicsDevice()->releaseTmpTexture(m_tmp_tex);
}

void fcGifContext::addFrameTask(int frame, bool local_palette, std::string &buf)
{
    int w = frame % m_conf.max_active_tasks;

    std::stringstream os(buf, std::ios::binary | std::ios::out);
    jo_gif_frame(os, frame, &m_gif, (unsigned char*)&m_work[w].raw[0], 3, local_palette);
    buf = os.str();
    fcAtomicDecrement(&m_active_task_count);
}

void fcGifContext::scrape()
{
    int min_frames = std::max<int>(m_conf.max_active_tasks, 1);

    // 最大フレーム数超えてたら間引く
    if (m_conf.max_frame > 0)
    {
        while (m_conf.max_frame > min_frames && m_frames.size() > m_conf.max_frame)
        {
            // 最初のフレームに含まれる情報削除すると読めなくなるので、2 番目以降から消す
            m_frames.erase(++m_frames.begin());
        }
    }

    // 最大容量超えてたら間引く
    if (m_conf.max_data_size > 0)
    {
        size_t size = 14; // gif header + footer size
        for (auto &i : m_frames) { size += i.size(); }
        while (m_frames.size() > min_frames && size > m_conf.max_data_size)
        {
            // 同上
            auto iter = ++m_frames.begin();
            size -= iter->size();
            m_frames.erase(iter);
        }
    }
}

void fcGifContext::addFrame(void *tex)
{
    if (m_tmp_tex==nullptr)
    {
        m_tmp_tex = fcGetGraphicsDevice()->createTmpTexture(m_conf.width, m_conf.height, fcE_ARGB32);
    }

    if (m_active_task_count >= m_conf.max_active_tasks)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (m_active_task_count >= m_conf.max_active_tasks)
        {
            m_tasks.wait();
        }
    }
    fcAtomicIncrement(&m_active_task_count);
    int frame = m_frame++;
    int w = frame % m_conf.max_active_tasks;

    // get raw framebuffer
    fcGetGraphicsDevice()->copyTextureData(&m_work[w].raw[0], tex, m_tmp_tex, m_conf.width, m_conf.height, fcE_ARGB32);

    scrape();
    m_frames.push_back(std::string());
    std::string& buf = m_frames.back();

    bool local_palette = frame==0 || (m_conf.keyframe != 0 && frame % m_conf.keyframe == 0);
    if (local_palette) {
        // パレットの新は前後のフレームに影響をあたえるため、同期更新でなければならない
        m_tasks.wait();
        addFrameTask(frame, local_palette, buf);
    }
    else
    {
        // compress asynchronously
        m_tasks.run([this, frame, local_palette, &buf](){ addFrameTask(frame, local_palette, buf); });
    }
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
