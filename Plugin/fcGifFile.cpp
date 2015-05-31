#include "pch.h"
#include "FrameCapturer.h"
#include "fcThreadPool.h"
#include "fcGraphicsDevice.h"
#include "jo_gif.cpp"

#ifdef fcSupportGIF

class fcGifContext
{
public:
    fcGifContext(fcGifConfig *conf);
    ~fcGifContext();
    bool addFrame(void *tex);
    void clearFrame();
    bool writeFile(const char *path, int begin_frame, int end_frame);
    int  writeMemory(void *buf, int begin_frame, int end_frame);

    int getFrameCount();
    void getFrameData(void *tex, int frame);
    int getExpectedDataSize(int begin_frame, int end_frame);
    void eraseFrame(int begin_frame, int end_frame);

private:
    void scrape(bool is_tasks_running);
    void addFrameTask(jo_gif_frame_t &o_fdata, const std::string &raw_buffer, int frame, bool local_palette);
    void write(std::ostream &os, int begin_frame, int end_frame);

private:
    fcEMagic m_magic; //  for debug
    fcGifConfig m_conf;
    std::vector<std::string> m_raw_buffers;
    std::list<jo_gif_frame_t> m_gif_buffers;
    jo_gif_t m_gif;
    int m_frame;
    fcTaskGroup m_tasks;
    std::atomic_int m_active_task_count;
};


fcGifContext::fcGifContext(fcGifConfig *conf)
    : m_magic(fcE_GifContext)
    , m_conf(*conf)
    , m_frame(0)
{
    m_active_task_count = 0;
    m_gif = jo_gif_start(m_conf.width, m_conf.height, 0, m_conf.num_colors);
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
            size += fdata.palette.size() + fdata.encoded.size() + 20;
        }

        while (m_gif_buffers.size() > min_frames && size > m_conf.max_data_size)
        {
            auto &fdata = m_gif_buffers.front();
            size -= fdata.palette.size() + fdata.encoded.size() + 20;
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
    // 実行中のタスクの数が上限に達している場合適当に待つ
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
    if (!fcGetGraphicsDevice()->readTexture(&raw_buffer[0], raw_buffer.size(), tex, m_conf.width, m_conf.height, fcE_ARGB32))
    {
        --frame;
        return false;
    }

    // gif データを生成
    m_gif_buffers.push_back(jo_gif_frame_t());
    jo_gif_frame_t& fdata = m_gif_buffers.back();
    bool local_palette = frame==0 || (m_conf.keyframe != 0 && frame % m_conf.keyframe == 0);
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

    scrape(true);
    return true;
}

void fcGifContext::clearFrame()
{
    m_tasks.wait();
    m_gif_buffers.clear();
    m_frame = 0;
}


static inline void adjust_frame(int &begin_frame, int &end_frame, int max_frame)
{
    begin_frame = std::max<int>(begin_frame, 0);
    if (end_frame < 0) {
        end_frame = max_frame;
    }
    else {
        end_frame = std::min<int>(end_frame, max_frame);
    }
}


void fcGifContext::write(std::ostream &os, int begin_frame, int end_frame)
{
    m_tasks.wait();
    scrape(false);

    adjust_frame(begin_frame, end_frame, (int)m_gif_buffers.size());
    auto begin = m_gif_buffers.begin();
    auto end = m_gif_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);

    // パレット探索
    auto palette = begin;
    while (palette->palette.empty()) { --palette; }

    int frame = 0;
    jo_gif_write_header(os, &m_gif);
    for (auto i = begin; i != end; ++i) {
        jo_gif_frame_t *pal = frame == 0 ? &(*palette) : nullptr;
        jo_gif_write_frame(os, &m_gif, &(*i), pal, frame++, m_conf.delay_csec);
    }
    jo_gif_write_footer(os, &m_gif);
}

bool fcGifContext::writeFile(const char *path, int begin_frame, int end_frame)
{
    std::ofstream os(path, std::ios::binary);
    if (!os) { return false; }
    write(os, begin_frame, end_frame);
    os.close();
    return true;
}

int fcGifContext::writeMemory(void *buf, int begin_frame, int end_frame)
{
    std::ostringstream os(std::ios::binary);
    write(os, begin_frame, end_frame);

    std::string s = os.str();
    memcpy(buf, &s[0], s.size());
    return (int)s.size();
}


int fcGifContext::getFrameCount()
{
    return m_gif_buffers.size();
}

void fcGifContext::getFrameData(void *tex, int frame)
{
    if (frame >= m_gif_buffers.size()) { return; }
    m_tasks.wait();
    scrape(false);

    jo_gif_frame_t *fdata, *palette;
    {
        auto it = m_gif_buffers.begin();
        std::advance(it, frame);
        fdata = &(*it);
        for (;; --it)
        {
            if (!it->palette.empty())
            {
                palette = &(*it);
                break;
            }
        }
    }

    std::string raw_pixels;
    raw_pixels.resize(m_conf.width * m_conf.height * 4);
    jo_gif_decode(&raw_pixels[0], fdata, palette);
    fcGetGraphicsDevice()->writeTexture(tex, m_gif.width, m_gif.height, fcE_ARGB32, &raw_pixels[0], raw_pixels.size());
}


int fcGifContext::getExpectedDataSize(int begin_frame, int end_frame)
{
    adjust_frame(begin_frame, end_frame, (int)m_gif_buffers.size());
    auto begin = m_gif_buffers.begin();
    auto end = m_gif_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);


    size_t size = 14; // gif header + footer size
    for (auto i = begin; i != end; ++i)
    {
        if (i == begin) {
            if (m_gif.repeat >= 0) { size += 19; }
            // パレット探索
            if (begin->palette.empty())
            {
                auto palette = begin;
                while (palette->palette.empty()) { --palette; }
                size += palette->palette.size();
            }
        }

        size += i->palette.size() + i->encoded.size() + 20;
    }
    return (int)size;
}

void fcGifContext::eraseFrame(int begin_frame, int end_frame)
{
    m_tasks.wait();
    scrape(false);

    adjust_frame(begin_frame, end_frame, (int)m_gif_buffers.size());
    auto begin = m_gif_buffers.begin();
    auto end = m_gif_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);
    m_gif_buffers.erase(begin, end);
}



#ifdef fcDebug
#define fcCheckContext(v) if(v==nullptr || *(fcEMagic*)v!=fcE_GifContext) { fcBreak(); }
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

fcCLinkage fcExport bool fcGifWriteFile(fcGifContext *ctx, const char *path, int begin_frame, int end_frame)
{
    fcCheckContext(ctx);
    return ctx->writeFile(path, begin_frame, end_frame);
}

fcCLinkage fcExport int fcGifWriteMemory(fcGifContext *ctx, void *buf, int begin_frame, int end_frame)
{
    fcCheckContext(ctx);
    return ctx->writeMemory(buf, begin_frame, end_frame);
}

fcCLinkage fcExport int fcGifGetFrameCount(fcGifContext *ctx)
{
    fcCheckContext(ctx);
    return ctx->getFrameCount();
}

fcCLinkage fcExport void fcGifGetFrameData(fcGifContext *ctx, void *tex, int frame)
{
    fcCheckContext(ctx);
    return ctx->getFrameData(tex, frame);
}

fcCLinkage fcExport int fcGifGetExpectedDataSize(fcGifContext *ctx, int begin_frame, int end_frame)
{
    fcCheckContext(ctx);
    return ctx->getExpectedDataSize(begin_frame, end_frame);
}

fcCLinkage fcExport void fcGifEraseFrame(fcGifContext *ctx, int begin_frame, int end_frame)
{
    fcCheckContext(ctx);
    ctx->eraseFrame(begin_frame, end_frame);
}

#endif // fcSupportGIF
