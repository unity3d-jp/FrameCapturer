#include "pch.h"
#include "FrameCapturer.h"

#ifdef fcSupportMP4
#include "fcFoundation.h"
#include "fcThreadPool.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcMP4File.h"
#include "fcColorSpace.h"
#include "fcH264Encoder.h"
#include "fcMP4Muxer.h"


class fcDummyMP4Context : public fcIMP4Context
{
public:
    void release() override { delete this; }

    bool addFrameTexture(void *tex) override { return false; }
    bool addFramePixels(void *pixels, fcEColorSpace cs) override { return false; }
    void clearFrame() override {}
    bool writeFile(const char *path, int begin_frame, int end_frame) override { return false; }
    int  writeMemory(void *buf, int begin_frame, int end_frame) override { return 0; }

    int getFrameCount() override { return 0; }
    void getFrameData(void *tex, int frame) override {}
    int getExpectedDataSize(int begin_frame, int end_frame) override { return 0; }
    void eraseFrame(int begin_frame, int end_frame) override {}
};


class fcMP4Context : public fcIMP4Context
{
public:
    struct H264FrameData
    {
        std::string data;
    };

    struct RawFrameData
    {
        std::string rgba;
        std::string i420;
    };

public:
    fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev);
    ~fcMP4Context();
    void release() override;

    bool addFrameTexture(void *tex) override;
    bool addFramePixels(void *pixels, fcEColorSpace cs) override;
    void clearFrame() override;
    bool writeFile(const char *path, int begin_frame, int end_frame) override;
    int  writeMemory(void *buf, int begin_frame, int end_frame) override;

    int getFrameCount() override;
    void getFrameData(void *tex, int frame) override;
    int getExpectedDataSize(int begin_frame, int end_frame) override;
    void eraseFrame(int begin_frame, int end_frame) override;

private:
    void scrape(bool is_tasks_running);
    void waitOneTask();
    void addFrameTask(H264FrameData &o_fdata, RawFrameData &raw_buffer, bool rgba2i420);
    void write(std::ostream &os, int begin_frame, int end_frame);

private:
    fcEMagic m_magic; //  for debug
    fcMP4Config m_conf;
    fcIGraphicsDevice *m_dev;
    std::vector<RawFrameData> m_raw_buffers;
    std::list<H264FrameData> m_h264_buffers;
    int m_frame;

    fcTaskGroup m_tasks;
    std::atomic<int> m_active_task_count;
};


fcIMP4Context* fcCreateMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev)
{
    return new fcMP4Context(conf, dev);
}


fcMP4Context::fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev)
    : m_magic(fcE_MP4Context)
    , m_conf(conf)
    , m_dev(dev)
    , m_frame(0)
    , m_active_task_count(0)
{
    m_raw_buffers.resize(m_conf.max_active_tasks);
    for (auto& rf : m_raw_buffers)
    {
        rf.rgba.resize(m_conf.width * m_conf.height * fcGetPixelSize(fcE_ARGB32));
        rf.i420.resize(m_conf.width * m_conf.height * 3 / 2);
    }
}

fcMP4Context::~fcMP4Context()
{
    m_tasks.wait();
    m_magic = fcE_Deleted;
}


void fcMP4Context::release()
{
    delete this;
}

void fcMP4Context::scrape(bool updating)
{
    // 最大容量か最大フレーム数が設定されている場合、それらを超過したフレームをここで切り捨てる。
    // 切り捨てるフレームがパレットを持っている場合パレットの移動も行う。

    // 実行中のタスクが更新中のデータを間引くのはマズいので、更新中は最低でもタスク数分は残す
    int min_frames = updating ? std::max<int>(m_conf.max_active_tasks, 1) : 1;

    // todo
}

fcThreadLocal fcH264Encoder *g_encoder;

void fcMP4Context::addFrameTask(H264FrameData &o_fdata, RawFrameData &raw, bool rgba2i420)
{
    if (g_encoder == nullptr) {
        g_encoder = new fcH264Encoder(m_conf.width, m_conf.height, m_conf.framerate, m_conf.bitrate);
    }
    int frame_size = m_conf.width * m_conf.height;
    uint8_t *y = (uint8_t*)&raw.i420[0];
    uint8_t *u = y + frame_size;
    uint8_t *v = u + (frame_size >> 2);
    if (rgba2i420) {
        RGBA_to_I420(y, u, v, (bRGBA*)&raw.rgba[0], m_conf.width, m_conf.height);
    }
    auto ret = g_encoder->encodeI420(y, u, v);
    o_fdata.data.assign((char*)ret.data, ret.size);
}

void fcMP4Context::waitOneTask()
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
}


bool fcMP4Context::addFrameTexture(void *tex)
{
    waitOneTask();
    int frame = m_frame++;
    RawFrameData& raw = m_raw_buffers[frame % m_conf.max_active_tasks];

    // フレームバッファの内容取得
    if (!m_dev->readTexture(&raw.rgba[0], raw.rgba.size(), tex, m_conf.width, m_conf.height, fcE_ARGB32))
    {
        --frame;
        return false;
    }

    // h264 データを生成
    m_h264_buffers.push_back(H264FrameData());
    H264FrameData& fdata = m_h264_buffers.back();
    ++m_active_task_count;
    m_tasks.run([this, &fdata, &raw](){
        addFrameTask(fdata, raw, true);
        --m_active_task_count;
    });

    scrape(true);
    return true;
}

bool fcMP4Context::addFramePixels(void *pixels, fcEColorSpace cs)
{
    waitOneTask();
    int frame = m_frame++;
    RawFrameData& raw = m_raw_buffers[frame % m_conf.max_active_tasks];

    bool rgba2i420 = true;
    if (cs == fcE_RGBA) {
        raw.rgba.assign((char*)pixels, raw.rgba.size());
    }
    else if (cs == fcE_I420) {
        rgba2i420 = false;
        raw.i420.assign((char*)pixels, m_conf.width*m_conf.height*3/2);
    }

    // h264 データを生成
    m_h264_buffers.push_back(H264FrameData());
    H264FrameData& fdata = m_h264_buffers.back();
    ++m_active_task_count;
    m_tasks.run([this, &fdata, &raw, rgba2i420](){
        addFrameTask(fdata, raw, rgba2i420);
        --m_active_task_count;
    });

    scrape(true);
    return true;
}

void fcMP4Context::clearFrame()
{
    m_tasks.wait();
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


void fcMP4Context::write(std::ostream &os, int begin_frame, int end_frame)
{
    m_tasks.wait();
    scrape(false);

    adjust_frame(begin_frame, end_frame, (int)m_h264_buffers.size());
    auto begin = m_h264_buffers.begin();
    auto end = m_h264_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);

    char tmp_h264_filename[256];
    char tmp_mp4_filename[256];
    sprintf(tmp_h264_filename, "%llu.h264", (uint64_t)::time(nullptr));
    sprintf(tmp_mp4_filename, "%llu.mp4", (uint64_t)::time(nullptr));
    {
        std::ofstream tmp_h264(tmp_h264_filename, std::ios::binary);
        for (auto i = begin; i != end; ++i) {
            tmp_h264.write(&i->data[0], i->data.size());
        }
    }
    {
        fcMP4Muxer muxer;
        muxer.mux(tmp_mp4_filename, tmp_h264_filename, m_conf.framerate);
    }
    {
        char buf[1024];
        std::ifstream tmp_mp4(tmp_mp4_filename, std::ios::binary);
        while (!tmp_mp4.eof()) {
            tmp_mp4.read(buf, sizeof(buf));
            os.write(buf, tmp_mp4.gcount());
        }
    }
    std::remove(tmp_h264_filename);
    std::remove(tmp_mp4_filename);
}

bool fcMP4Context::writeFile(const char *path, int begin_frame, int end_frame)
{
    std::ofstream os(path, std::ios::binary);
    if (!os) { return false; }
    write(os, begin_frame, end_frame);
    os.close();
    return true;
}

int fcMP4Context::writeMemory(void *buf, int begin_frame, int end_frame)
{
    std::ostringstream os(std::ios::binary);
    write(os, begin_frame, end_frame);

    std::string s = os.str();
    memcpy(buf, &s[0], s.size());
    return (int)s.size();
}


int fcMP4Context::getFrameCount()
{
    return m_h264_buffers.size();
}

void fcMP4Context::getFrameData(void *tex, int frame)
{
    if (frame >= m_h264_buffers.size()) { return; }
    m_tasks.wait();
    scrape(false);

    H264FrameData *fdata = nullptr;
    {
        auto it = m_h264_buffers.begin();
        std::advance(it, frame);
        fdata = &(*it);
    }

    RawFrameData raw;
    raw.rgba.resize(m_conf.width * m_conf.height * 4);
    raw.i420.resize(m_conf.width * m_conf.height * 3/2);
    // todo: decode
    m_dev->writeTexture(tex, m_conf.width, m_conf.height, fcE_ARGB32, &raw.rgba[0], raw.rgba.size());
}


int fcMP4Context::getExpectedDataSize(int begin_frame, int end_frame)
{
    adjust_frame(begin_frame, end_frame, (int)m_h264_buffers.size());
    auto begin = m_h264_buffers.begin();
    auto end = m_h264_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);


    size_t size = 0;
    // todo
    return (int)size;
}

void fcMP4Context::eraseFrame(int begin_frame, int end_frame)
{
    m_tasks.wait();
    scrape(false);

    adjust_frame(begin_frame, end_frame, (int)m_h264_buffers.size());
    auto begin = m_h264_buffers.begin();
    auto end = m_h264_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);
    m_h264_buffers.erase(begin, end);
}


#endif // fcSupportMP4
