#include "pch.h"
#include "fcFoundation.h"
#include "fcThreadPool.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcGifFile.h"
#include "external/jo_gif.cpp"

#ifdef fcSupportHalfPixelFormat
    #include <half.h>
    #ifdef fcWindows
        #pragma comment(lib, "Half.lib")
    #endif
#endif // fcSupportHalfPixelFormat


struct fcGifTaskData
{
    fcPixelFormat raw_pixel_format;
    std::vector<uint8_t> raw_pixels;
    std::vector<uint8_t> rgba8_pixels;
    jo_gif_frame_t *gif_frame;
    int frame;
    bool local_palette;
    fcTime timestamp;
};

class fcGifContext : public fcIGifContext
{
public:
    fcGifContext(const fcGifConfig &conf, fcIGraphicsDevice *dev);
    ~fcGifContext();
    void release() override;

    bool addFrameTexture(void *tex, fcTextureFormat fmt, bool keyframe, fcTime timestamp) override;
    bool addFramePixels(const void *pixels, fcPixelFormat fmt, bool keyframe, fcTime timestamp) override;
    void clearFrame() override;
    bool writeFile(const char *path, int begin_frame, int end_frame) override;
    int  writeMemory(void *buf, int begin_frame, int end_frame) override;

    int  getFrameCount() override;
    void getFrameData(void *tex, int frame) override;
    int  getExpectedDataSize(int begin_frame, int end_frame) override;
    void eraseFrame(int begin_frame, int end_frame) override;

private:
    fcGifTaskData&  getTempraryVideoFrame();
    void            returnTempraryVideoFrame(fcGifTaskData& v);

    void scrape(bool is_tasks_running);
    void addGifFrame(fcGifTaskData& data);
    void kickTask(fcGifTaskData& data);
    void write(std::ostream &os, int begin_frame, int end_frame);

private:
    fcGifConfig m_conf;
    fcIGraphicsDevice *m_dev;
    std::vector<fcGifTaskData> m_buffers;
    std::vector<fcGifTaskData*> m_buffers_unused;
    std::list<jo_gif_frame_t> m_gif_buffers;
    jo_gif_t m_gif;
    fcTaskGroup m_tasks;
    std::mutex m_mutex;
    int m_frame;
};


fcGifContext::fcGifContext(const fcGifConfig &conf, fcIGraphicsDevice *dev)
    : m_conf(conf)
    , m_dev(dev)
    , m_frame()
{
    m_gif = jo_gif_start(m_conf.width, m_conf.height, 0, m_conf.num_colors);

    // allocate working buffers
    m_buffers.resize(m_conf.max_active_tasks);
    for (auto& buf : m_buffers)
    {
        buf.rgba8_pixels.resize(m_conf.width * m_conf.height * fcGetPixelSize(fcTextureFormat_ARGB32));
        m_buffers_unused.push_back(&buf);
    }
}

fcGifContext::~fcGifContext()
{
    m_tasks.wait();
    jo_gif_end(&m_gif);
}


void fcGifContext::release()
{
    delete this;
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
        while (m_conf.max_frame > min_frames && m_gif_buffers.size() > size_t(m_conf.max_frame))
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

        while (m_gif_buffers.size() > size_t(min_frames) && size > size_t(m_conf.max_data_size))
        {
            auto &fdata = m_gif_buffers.front();
            size -= fdata.palette.size() + fdata.encoded.size() + 20;
            advance_palette_and_pop_front(m_gif_buffers);
        }
    }
}

fcGifTaskData& fcGifContext::getTempraryVideoFrame()
{
    fcGifTaskData *ret = nullptr;

    // wait if all temporaries are in use
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_buffers_unused.empty()) {
                ret = m_buffers_unused.back();
                m_buffers_unused.pop_back();
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return *ret;
}

void fcGifContext::returnTempraryVideoFrame(fcGifTaskData& v)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_buffers_unused.push_back(&v);
}

void fcGifContext::addGifFrame(fcGifTaskData& data)
{
    unsigned char *src = nullptr;
    if (data.raw_pixel_format == fcPixelFormat_RGBA8) {
        src = (unsigned char*)&data.raw_pixels[0];
    }
    else {
        // convert pixel format

        size_t psize = fcGetPixelSize(data.raw_pixel_format);
        size_t n = data.raw_pixels.size() / psize;

        switch (data.raw_pixel_format) {
        case fcPixelFormat_RGB8:
            for (size_t i = 0; i < n; ++i) {
                data.rgba8_pixels[i * 4 + 0] = data.raw_pixels[i * 3 + 0];
                data.rgba8_pixels[i * 4 + 1] = data.raw_pixels[i * 3 + 1];
                data.rgba8_pixels[i * 4 + 2] = data.raw_pixels[i * 3 + 2];
                data.rgba8_pixels[i * 4 + 3] = 255;
            }
            break;
        case fcPixelFormat_RGBFloat:
            for (size_t i = 0; i < n; ++i) {
                data.rgba8_pixels[i * 4 + 0] = uint8_t((float&)data.raw_pixels[i * 12 + 0] * 255.0f);
                data.rgba8_pixels[i * 4 + 1] = uint8_t((float&)data.raw_pixels[i * 12 + 4] * 255.0f);
                data.rgba8_pixels[i * 4 + 2] = uint8_t((float&)data.raw_pixels[i * 12 + 8] * 255.0f);
                data.rgba8_pixels[i * 4 + 3] = 255;
            }
            break;
        case fcPixelFormat_RGBAFloat:
            for (size_t i = 0; i < n; ++i) {
                data.rgba8_pixels[i * 4 + 0] = uint8_t((float&)data.raw_pixels[i * 16 + 0] * 255.0f);
                data.rgba8_pixels[i * 4 + 1] = uint8_t((float&)data.raw_pixels[i * 16 + 4] * 255.0f);
                data.rgba8_pixels[i * 4 + 2] = uint8_t((float&)data.raw_pixels[i * 16 + 8] * 255.0f);
                data.rgba8_pixels[i * 4 + 3] = uint8_t((float&)data.raw_pixels[i * 16 +12] * 255.0f);
            }
            break;
#ifdef fcSupportHalfPixelFormat
        case fcPixelFormat_RGBHalf:
            for (size_t i = 0; i < n; ++i) {
                data.rgba8_pixels[i * 4 + 0] = uint8_t((half&)data.raw_pixels[i * 6 + 0] * 255.0f);
                data.rgba8_pixels[i * 4 + 1] = uint8_t((half&)data.raw_pixels[i * 6 + 2] * 255.0f);
                data.rgba8_pixels[i * 4 + 2] = uint8_t((half&)data.raw_pixels[i * 6 + 4] * 255.0f);
                data.rgba8_pixels[i * 4 + 3] = 255;
            }
            break;
        case fcPixelFormat_RGBAHalf:
            for (size_t i = 0; i < n; ++i) {
                data.rgba8_pixels[i * 4 + 0] = uint8_t((half&)data.raw_pixels[i * 8 + 0] * 255.0f);
                data.rgba8_pixels[i * 4 + 1] = uint8_t((half&)data.raw_pixels[i * 8 + 2] * 255.0f);
                data.rgba8_pixels[i * 4 + 2] = uint8_t((half&)data.raw_pixels[i * 8 + 4] * 255.0f);
                data.rgba8_pixels[i * 4 + 3] = uint8_t((half&)data.raw_pixels[i * 8 + 6] * 255.0f);
            }
            break;
#endif // fcSupportHalfPixelFormat
        }
        src = (unsigned char*)&data.rgba8_pixels[0];
    }

    jo_gif_frame(&m_gif, data.gif_frame, src, data.frame, data.local_palette);
    returnTempraryVideoFrame(data);
}

void fcGifContext::kickTask(fcGifTaskData& data)
{
    // gif データを生成
    m_gif_buffers.push_back(jo_gif_frame_t());
    data.gif_frame = &m_gif_buffers.back();
    data.frame = m_frame++;
    data.local_palette = data.frame == 0 || (m_conf.keyframe != 0 && data.frame % m_conf.keyframe == 0);

    if (data.local_palette) {
        // パレットの更新は前後のフレームに影響をあたえるため、同期更新でなければならない
        m_tasks.wait();
        addGifFrame(data);
    }
    else
    {
        m_tasks.run([this, &data]() {
            addGifFrame(data);
        });
    }

    scrape(true);
}

bool fcGifContext::addFrameTexture(void *tex, fcTextureFormat fmt, bool keyframe, fcTime timestamp)
{
    fcGifTaskData& data = getTempraryVideoFrame();
    data.timestamp = timestamp;
    data.local_palette = data.frame == 0 || keyframe;

    // フレームバッファの内容取得
    data.raw_pixels.resize(m_conf.width * m_conf.height * fcGetPixelSize(fmt));
    data.raw_pixel_format = fcGetPixelFormat(fmt);
    if (!m_dev->readTexture(&data.raw_pixels[0], data.raw_pixels.size(), tex, m_conf.width, m_conf.height, fmt))
    {
        return false;
    }

    kickTask(data);
    return true;
}

bool fcGifContext::addFramePixels(const void *pixels, fcPixelFormat fmt, bool keyframe, fcTime timestamp)
{
    fcGifTaskData& data = getTempraryVideoFrame();
    data.timestamp = timestamp;
    data.raw_pixel_format = fmt;
    data.raw_pixels.assign((char*)pixels, (char*)pixels + (m_conf.width * m_conf.height * fcGetPixelSize(fmt)));

    kickTask(data);
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
    return (int)m_gif_buffers.size();
}

void fcGifContext::getFrameData(void *tex, int frame)
{
    if (frame >= 0 && size_t(frame) >= m_gif_buffers.size()) { return; }
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
    m_dev->writeTexture(tex, m_gif.width, m_gif.height, fcTextureFormat_ARGB32, &raw_pixels[0], raw_pixels.size());
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


fcCLinkage fcExport fcIGifContext* fcGifCreateContextImpl(const fcGifConfig &conf, fcIGraphicsDevice *dev)
{
    return new fcGifContext(conf, dev);
}
