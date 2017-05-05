#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcGifContext.h"

#ifdef fcSupportGIF
#include "external/jo_gif.cpp"

typedef jo_gif_frame_t fcGifFrame;

struct fcGifTaskData
{
    fcPixelFormat raw_pixel_format = fcPixelFormat_Unknown;
    Buffer raw_pixels;
    Buffer rgba8_pixels;
    fcGifFrame *gif_frame = nullptr;
    int frame = 0;
    bool local_palette = true;
    fcTime timestamp = 0.0;
};

class fcGifContext : public fcIGifContext
{
public:
    fcGifContext(const fcGifConfig &conf, fcIGraphicsDevice *dev);
    ~fcGifContext();
    void release() override;

    void addOutputStream(fcStream *s) override;
    bool addFrameTexture(void *tex, fcPixelFormat fmt, bool keyframe, fcTime timestamp) override;
    bool addFramePixels(const void *pixels, fcPixelFormat fmt, bool keyframe, fcTime timestamp) override;

private:
    fcGifTaskData&  getTempraryVideoFrame();
    void            returnTempraryVideoFrame(fcGifTaskData& v);

    void addGifFrame(fcGifTaskData& data);
    void kickTask(fcGifTaskData& data);
    bool flush();

private:
    fcGifConfig m_conf;
    fcIGraphicsDevice *m_dev = nullptr;
    std::vector<fcStream*> m_streams;
    std::vector<fcGifTaskData> m_buffers;
    std::vector<fcGifTaskData*> m_buffers_unused;
    std::list<fcGifFrame> m_gif_frames;
    jo_gif_t m_gif;
    fcTaskGroup m_tasks;
    std::mutex m_mutex;
    int m_frame = 0;
};


fcGifContext::fcGifContext(const fcGifConfig &conf, fcIGraphicsDevice *dev)
    : m_conf(conf)
    , m_dev(dev)
{
    m_gif = jo_gif_start(m_conf.width, m_conf.height, 0, m_conf.num_colors);

    // allocate working buffers
    if (m_conf.max_active_tasks <= 0) {
        m_conf.max_active_tasks = std::thread::hardware_concurrency();
    }
    m_buffers.resize(m_conf.max_active_tasks);
    for (auto& buf : m_buffers)
    {
        buf.rgba8_pixels.resize(m_conf.width * m_conf.height * fcGetPixelSize(fcPixelFormat_RGBAu8));
        m_buffers_unused.push_back(&buf);
    }
}

fcGifContext::~fcGifContext()
{
    m_tasks.wait();
    flush();

    jo_gif_end(&m_gif);
}


void fcGifContext::release()
{
    delete this;
}

void fcGifContext::addOutputStream(fcStream *os)
{
    if (!os) { return; }
    m_streams.push_back(os);
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
    if (data.raw_pixel_format == fcPixelFormat_RGBAu8) {
        src = (unsigned char*)&data.raw_pixels[0];
    }
    else {
        // convert pixel format
        size_t npixels = data.raw_pixels.size() / fcGetPixelSize(data.raw_pixel_format);
        fcConvertPixelFormat(&data.rgba8_pixels[0], fcPixelFormat_RGBAu8, &data.raw_pixels[0], data.raw_pixel_format, npixels);
        src = (unsigned char*)&data.rgba8_pixels[0];
    }

    jo_gif_frame(&m_gif, data.gif_frame, src, data.frame, data.local_palette);
    returnTempraryVideoFrame(data);
}

void fcGifContext::kickTask(fcGifTaskData& data)
{
    m_gif_frames.push_back(fcGifFrame());
    data.gif_frame = &m_gif_frames.back();
    data.gif_frame->timestamp = data.timestamp;
    data.frame = m_frame++;

    if (data.local_palette) {
        // updating palette must be in sync
        m_tasks.wait();
        addGifFrame(data);
    }
    else
    {
        m_tasks.run([this, &data]() {
            addGifFrame(data);
        });
    }
}

bool fcGifContext::addFrameTexture(void *tex, fcPixelFormat fmt, bool keyframe, fcTime timestamp)
{
    if (m_dev == nullptr) {
        fcDebugLog("fcGifContext::addFrameTexture(): gfx device is null.");
        return false;
    }
    fcGifTaskData& data = getTempraryVideoFrame();
    data.timestamp = timestamp >= 0.0 ? timestamp : GetCurrentTimeInSeconds();
    data.local_palette = data.frame == 0 || keyframe;

    data.raw_pixels.resize(m_conf.width * m_conf.height * fcGetPixelSize(fmt));
    data.raw_pixel_format = fmt;
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
    data.timestamp = timestamp >= 0.0 ? timestamp : GetCurrentTimeInSeconds();
    data.local_palette = data.frame == 0 || keyframe;
    data.raw_pixel_format = fmt;
    data.raw_pixels.assign((char*)pixels, m_conf.width * m_conf.height * fcGetPixelSize(fmt));

    kickTask(data);
    return true;
}

bool fcGifContext::flush()
{
    m_tasks.wait();

    int frame = 0;
    for(auto os : m_streams) jo_gif_write_header(*os, &m_gif);
    for (auto i = m_gif_frames.begin(); i != m_gif_frames.end(); ++i) {
        auto next = i; ++next;
        int duration = 1; // unit: centi-second
        if (next != m_gif_frames.end()) {
            duration = int((next->timestamp - i->timestamp) * 100.0); // seconds to centi-seconds
        }
        for (auto os : m_streams) jo_gif_write_frame(*os, &m_gif, &(*i), nullptr, frame++, duration);
    }
    for (auto os : m_streams) jo_gif_write_footer(*os, &m_gif);

    return true;
}


fcIGifContext* fcGifCreateContextImpl(const fcGifConfig &conf, fcIGraphicsDevice *dev)
{
    return new fcGifContext(conf, dev);
}

#else // fcSupportGIF

fcIGifContext* fcGifCreateContextImpl(const fcGifConfig &conf, fcIGraphicsDevice *dev)
{
    return nullptr;
}

#endif // fcSupportGIF
