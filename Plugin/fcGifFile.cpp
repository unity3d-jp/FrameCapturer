#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"
#include "jo_gif.cpp"

class fcGifContext
{
public:
    fcGifContext(fcGifConfig *conf);
    ~fcGifContext();
    bool addFrame(void *tex);
    void clearFrame();
    bool writeFile(const char *path);

private:
    void scrape(bool is_tasks_running);
    void addFrameTask(std::string &o_gif_buffer, const std::string &raw_buffer, int frame, bool local_palette);

private:
    int m_magic; //  for debug
    fcGifConfig m_conf;
    std::vector<std::string> m_raw_buffers;
    std::list<std::string> m_gif_buffers;
    tbb::task_group m_tasks;
    jo_gif_t m_gif;
    int m_frame;
    std::atomic_int m_active_task_count;
};


fcGifContext::fcGifContext(fcGifConfig *conf)
    : m_magic(fcE_GifContext)
    , m_conf(*conf)
    , m_frame(0)
{
    m_gif = jo_gif_start(m_conf.width, m_conf.height, 0, 255);
    m_raw_buffers.resize(m_conf.max_active_tasks);
    for (auto& rf : m_raw_buffers)
    {
        rf.resize(m_conf.width * m_conf.height * fcGetPixelSize(fcE_ARGB32));
    }
}

fcGifContext::~fcGifContext()
{
    m_tasks.wait();
    jo_gif_end(&m_gif);
}

void fcGifContext::scrape(bool updating)
{
    // 実行中のタスクが更新中のデータを間引くのはマズいので、更新中は最低でもタスク数分は残す
    int min_frames = updating ? std::max<int>(m_conf.max_active_tasks, 1) : 1;

    // 最大フレーム数超えてたら間引く
    if (m_conf.max_frame > 0)
    {
        while (m_conf.max_frame > min_frames && m_gif_buffers.size() > m_conf.max_frame)
        {
            // 最初のフレームに含まれる情報削除すると読めなくなるので、2 番目以降から消す
            m_gif_buffers.erase(++m_gif_buffers.begin());
        }
    }

    // 最大容量超えてたら間引く
    if (m_conf.max_data_size > 0)
    {
        size_t size = 14; // gif header + footer size
        for (auto &i : m_gif_buffers) { size += i.size(); }
        while (m_gif_buffers.size() > min_frames && size > m_conf.max_data_size)
        {
            // 同上
            auto iter = ++m_gif_buffers.begin();
            size -= iter->size();
            m_gif_buffers.erase(iter);
        }
    }
}

void fcGifContext::addFrameTask(std::string &o_gif_buffer, const std::string &raw_buffer, int frame, bool local_palette)
{
    std::stringstream os(std::ios::binary | std::ios::out);
    jo_gif_frame(os, frame, &m_gif, (unsigned char*)&raw_buffer[0], m_conf.delay_csec, local_palette);
    o_gif_buffer = os.str();
}

bool fcGifContext::addFrame(void *tex)
{
    if (m_active_task_count >= m_conf.max_active_tasks)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (m_active_task_count >= m_conf.max_active_tasks)
        {
            m_tasks.wait();
        }
    }
    int frame = m_frame++;

    // フレームバッファの内容取得
    std::string& raw_buffer = m_raw_buffers[frame % m_conf.max_active_tasks];
    if (!fcGetGraphicsDevice()->copyTextureData(&raw_buffer[0], raw_buffer.size(), tex, m_conf.width, m_conf.height, fcE_ARGB32))
    {
        --frame;
        return false;
    }

    // gif データを生成
    m_gif_buffers.push_back(std::string());
    std::string& gif_buffer = m_gif_buffers.back();
    bool local_palette = frame==0 || (m_conf.keyframe != 0 && frame % m_conf.keyframe == 0);
    if (local_palette) {
        // パレットの更新は前後のフレームに影響をあたえるため、同期更新でなければならない
        m_tasks.wait();
        addFrameTask(gif_buffer, raw_buffer, frame, local_palette);
    }
    else
    {
        ++m_active_task_count;
        m_tasks.run([this, &gif_buffer, &raw_buffer, frame, local_palette](){
            addFrameTask(gif_buffer, raw_buffer, frame, local_palette);
            --m_active_task_count;
        });
    }

    scrape(true);
    return true;
}

void fcGifContext::clearFrame()
{
    m_tasks.wait();
    m_gif_buffers.clear();
    m_frame = 0;
}

bool fcGifContext::writeFile(const char *path)
{
    m_tasks.wait();
    scrape(false);

    FILE *fout = fopen(path, "wb");
    if (fout == nullptr) { return false; }

    jo_gif_write_header(&m_gif, fout);
    for (auto &buf : m_gif_buffers) {
        fwrite(buf.c_str(), buf.size(), 1, fout);
    }
    jo_gif_write_footer(&m_gif, fout);
    fclose(fout);
    return true;
}



#ifdef fcDebug
#define fcCheckContext(v) if(v==nullptr || *(int*)v!=fcE_GifContext) { fcBreak(); }
#else  // fcDebug
#define fcCheckContext(v) 
#endif // fcDebug


fcCLinkage fcExport fcGifContext* fcGifCreateContext(fcGifConfig *conf)
{
    return new fcGifContext(conf);
}

fcCLinkage fcExport void fcGifDestroyContext(fcGifContext *ctx)
{
    fcCheckContext(ctx);
    delete ctx;
}

fcCLinkage fcExport bool fcGifAddFrame(fcGifContext *ctx, void *tex)
{
    fcCheckContext(ctx);
    return ctx->addFrame(tex);
}

fcCLinkage fcExport void fcGifClearFrame(fcGifContext *ctx)
{
    fcCheckContext(ctx);
    ctx->clearFrame();
}

fcCLinkage fcExport bool fcGifWriteFile(fcGifContext *ctx, const char *path)
{
    fcCheckContext(ctx);
    return ctx->writeFile(path);
}
