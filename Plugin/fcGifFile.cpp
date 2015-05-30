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
    void addFrameTask(jo_gif_frame_t &o_fdata, const std::string &raw_buffer, int frame, bool local_palette);

private:
    int m_magic; //  for debug
    fcGifConfig m_conf;
    std::vector<std::string> m_raw_buffers;
    std::list<jo_gif_frame_t> m_gif_buffers;
    jo_gif_t m_gif;
    int m_frame;
#ifdef fcWithTBB
    tbb::task_group m_tasks;
    std::atomic_int m_active_task_count;
#endif
};


fcGifContext::fcGifContext(fcGifConfig *conf)
    : m_magic(fcE_GifContext)
    , m_conf(*conf)
    , m_frame(0)
{
    m_gif = jo_gif_start(m_conf.width, m_conf.height, 0, m_conf.num_colors);
    m_raw_buffers.resize(m_conf.max_active_tasks);
    for (auto& rf : m_raw_buffers)
    {
        rf.resize(m_conf.width * m_conf.height * fcGetPixelSize(fcE_ARGB32));
    }
}

fcGifContext::~fcGifContext()
{
#ifdef fcWithTBB
    m_tasks.wait();
#endif
    jo_gif_end(&m_gif);
}


static inline void advance_palette_and_pop_front(std::list<jo_gif_frame_t>& frames)
{
    // 先頭のパレットをいっこ先のフレームへ移動
    if (frames.size() >= 2)
    {
        auto &first = frames.front();
        auto &second = *(++frames.begin());
        if (!first.palette.empty() && second.palette.empty()) {
            second.palette = first.palette;
        }
    }
    frames.pop_front();
}

void fcGifContext::scrape(bool updating)
{
    // 最大容量か最大フレーム数が設定されている場合、それらを超過したフレームをここで切り捨てる。
    // 切り捨てるフレームがパレットを持っている場合パレットの移動も行う。

    // 実行中のタスクが更新中のデータを間引くのはマズいので、更新中は最低でもタスク数分は残す
    int min_frames = updating ? std::max<int>(m_conf.max_active_tasks, 1) : 1;

    // 最大フレーム数超えてたら間引く
    if (m_conf.max_frame > 0)
    {
        while (m_conf.max_frame > min_frames && m_gif_buffers.size() > m_conf.max_frame)
        {
            advance_palette_and_pop_front(m_gif_buffers);
        }
    }

    // 最大容量超えてたら間引く
    if (m_conf.max_data_size > 0)
    {
        size_t size = 14; // gif header + footer size
        if (m_gif.repeat >= 0) { size += 19; }
        for (auto &fdata : m_gif_buffers)
        {
            size += fdata.palette.size() + fdata.pixels.size() + 20;
        }

        while (m_gif_buffers.size() > min_frames && size > m_conf.max_data_size)
        {
            auto &fdata = m_gif_buffers.front();
            size -= fdata.palette.size() + fdata.pixels.size() + 20;
            advance_palette_and_pop_front(m_gif_buffers);
        }
    }
}

void fcGifContext::addFrameTask(jo_gif_frame_t &o_fdata, const std::string &raw_buffer, int frame, bool local_palette)
{
    jo_gif_frame(&m_gif, &o_fdata, (unsigned char*)&raw_buffer[0], frame, local_palette);
}

bool fcGifContext::addFrame(void *tex)
{
#ifdef fcWithTBB
    // 実行中のタスクの数が上限に達している場合適当に待つ
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

    // フレームバッファの内容取得
    std::string& raw_buffer = m_raw_buffers[frame % m_conf.max_active_tasks];
    if (!fcGetGraphicsDevice()->copyTextureData(&raw_buffer[0], raw_buffer.size(), tex, m_conf.width, m_conf.height, fcE_ARGB32))
    {
        --frame;
        return false;
    }

    // gif データを生成
    m_gif_buffers.push_back(jo_gif_frame_t());
    jo_gif_frame_t& fdata = m_gif_buffers.back();
    bool local_palette = frame==0 || (m_conf.keyframe != 0 && frame % m_conf.keyframe == 0);
#ifdef fcWithTBB
    if (local_palette) {
        // パレットの更新は前後のフレームに影響をあたえるため、同期更新でなければならない
        m_tasks.wait();
        addFrameTask(fdata, raw_buffer, frame, local_palette);
    }
    else
    {
        ++m_active_task_count;
        m_tasks.run([this, &fdata, &raw_buffer, frame, local_palette](){
            addFrameTask(fdata, raw_buffer, frame, local_palette);
            --m_active_task_count;
        });
    }
#else
    addFrameTask(fdata, raw_buffer, frame, local_palette);
#endif

    scrape(true);
    return true;
}

void fcGifContext::clearFrame()
{
#ifdef fcWithTBB
    m_tasks.wait();
#endif
    m_gif_buffers.clear();
    m_frame = 0;
}

bool fcGifContext::writeFile(const char *path)
{
#ifdef fcWithTBB
    m_tasks.wait();
#endif
    scrape(false);

    std::ofstream fout(path, std::ios::binary);
    if (!fout) { return false; }

    int frame = 0;
    jo_gif_write_header(fout, &m_gif);
    for (auto &fdata : m_gif_buffers) {
        jo_gif_write_frame(fout, &m_gif, &fdata, frame++, m_conf.delay_csec);
    }
    jo_gif_write_footer(fout, &m_gif);
    fout.close();
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
