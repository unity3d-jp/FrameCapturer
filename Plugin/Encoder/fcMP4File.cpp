#include "pch.h"
#include "FrameCapturer.h"

#ifdef fcSupportMP4
#include <libyuv/libyuv.h>
#include "fcFoundation.h"
#include "fcThreadPool.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcMP4File.h"
#include "fcH264Encoder.h"
#include "fcMP4Muxer.h"
#ifdef fcWindows
#pragma comment(lib, "yuv.lib")
#endif


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
    void enqueueTask(const std::function<void()> &f);
    void processTasks();

    void resetEncoder();
    void scrape(bool is_tasks_running);
    void wait();
    void waitOne();
    void addFrameTask(H264FrameData &o_fdata, RawFrameData &raw_buffer, bool rgba2i420);
    void write(std::ostream &os, int begin_frame, int end_frame);

private:
    fcEMagic m_magic; //  for debug
    fcMP4Config m_conf;
    fcIGraphicsDevice *m_dev;
    std::vector<RawFrameData> m_raw_buffers;
    std::list<H264FrameData> m_h264_buffers;
    int m_frame;

    std::unique_ptr<fcH264Encoder> m_encoder;
    std::unique_ptr<fcMP4Muxer> m_muxer;

    std::atomic<int> m_active_task_count;
    std::thread m_worker;
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    std::deque<std::function<void()>> m_tasks;
    bool m_stop;
};


fcMP4Context::fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev)
    : m_magic(fcE_MP4Context)
    , m_conf(conf)
    , m_dev(dev)
    , m_frame(0)
    , m_active_task_count(0)
    , m_stop(false)
{
    // allocate working buffer
    m_raw_buffers.resize(m_conf.max_buffers);
    for (auto& rf : m_raw_buffers)
    {
        rf.rgba.resize(m_conf.width * m_conf.height * 4);
        rf.i420.resize(roundup<2>(m_conf.width) * roundup<2>(m_conf.height) * 3 / 2);
    }

    resetEncoder();
    m_muxer.reset(new fcMP4Muxer());

    // run working thread
    m_worker = std::thread([this](){ processTasks(); });
}

fcMP4Context::~fcMP4Context()
{
    // stop working thread
    m_stop = true;
    m_condition.notify_all();
    m_worker.join();

    m_magic = fcE_Deleted;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void fcMP4Context::resetEncoder()
{
    wait();
    m_encoder.reset();
    m_encoder.reset(new fcH264Encoder(m_conf.width, m_conf.height, m_conf.framerate, m_conf.bitrate));
}

void fcMP4Context::enqueueTask(const std::function<void()> &f)
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_tasks.push_back(std::function<void()>(f));
    }
    m_condition.notify_one();
}

void fcMP4Context::processTasks()
{
    while (!m_stop)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            while (!m_stop && m_tasks.empty()) {
                m_condition.wait(lock);
            }
            if (m_stop) { return; }

            task = m_tasks.front();
            m_tasks.pop_front();
        }
        task();
    }
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
    int min_frames = updating ? std::max<int>(m_conf.max_buffers, 1) : 1;

    // todo
}

void fcMP4Context::addFrameTask(H264FrameData &o_fdata, RawFrameData &raw, bool rgba2i420)
{
    // 必要であれば RGBA -> I420 変換
    int width = m_conf.width;
    int frame_size = m_conf.width * m_conf.height;
    uint8 *y = (uint8*)&raw.i420[0];
    uint8 *u = y + frame_size;
    uint8 *v = u + (frame_size >> 2);
    if (rgba2i420) {
        libyuv::ABGRToI420(
            (uint8*)&raw.rgba[0], width * 4,
            y, width,
            u, width >> 1,
            v, width >> 1,
            m_conf.width, m_conf.height );
    }

    // I420 のピクセルデータを H264 へエンコード
    auto ret = m_encoder->encodeI420(y, u, v);
    o_fdata.data.assign((char*)ret.data, ret.size);
}

void fcMP4Context::wait()
{
    while (m_active_task_count > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void fcMP4Context::waitOne()
{
    // 実行中のタスクの数が上限に達している場合適当に待つ
    while (m_active_task_count >= m_conf.max_buffers)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


bool fcMP4Context::addFrameTexture(void *tex)
{
    waitOne();
    int frame = m_frame++;
    RawFrameData& raw = m_raw_buffers[frame % m_conf.max_buffers];

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
    enqueueTask([this, &fdata, &raw](){
        addFrameTask(fdata, raw, true);
        --m_active_task_count;
    });

    scrape(true);
    return true;
}

bool fcMP4Context::addFramePixels(void *pixels, fcEColorSpace cs)
{
    waitOne();
    int frame = m_frame++;
    RawFrameData& raw = m_raw_buffers[frame % m_conf.max_buffers];

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
    enqueueTask([this, &fdata, &raw, rgba2i420](){
        addFrameTask(fdata, raw, rgba2i420);
        --m_active_task_count;
    });

    scrape(true);
    return true;
}

void fcMP4Context::clearFrame()
{
    wait();
    m_frame = 0;
    m_h264_buffers.clear();
    resetEncoder();
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
    wait();
    scrape(false);

    adjust_frame(begin_frame, end_frame, (int)m_h264_buffers.size());
    auto begin = m_h264_buffers.begin();
    auto end = m_h264_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);

    char tmp_h264_filename[256];
    char tmp_mp4_filename[256];
    uint64_t now = (uint64_t)::time(nullptr);
    sprintf(tmp_h264_filename, "%llu.h264", now);
    sprintf(tmp_mp4_filename, "%llu.mp4", now);
    {
        std::ofstream tmp_h264(tmp_h264_filename, std::ios::binary);
        for (auto i = begin; i != end; ++i) {
            tmp_h264.write(&i->data[0], i->data.size());
        }
    }
    {
        m_muxer->mux(tmp_mp4_filename, tmp_h264_filename, m_conf.framerate);
    }
    {
        char buf[1024];
        std::ifstream tmp_mp4(tmp_mp4_filename, std::ios::binary);
        while (!tmp_mp4.eof()) {
            tmp_mp4.read(buf, sizeof(buf));
            os.write(buf, tmp_mp4.gcount());
        }
    }
#ifdef fcMaster
    std::remove(tmp_h264_filename);
    std::remove(tmp_mp4_filename);
#endif // fcMaster
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
    wait();
    scrape(false);

    H264FrameData *fdata = nullptr;
    {
        auto it = m_h264_buffers.begin();
        std::advance(it, frame);
        fdata = &(*it);
    }

    RawFrameData raw;
    raw.rgba.resize(m_conf.width * m_conf.height * 4);
    raw.i420.resize(roundup<2>(m_conf.width) * roundup<2>(m_conf.height) * 3 / 2);
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

    size_t size = 2048; // mp4 metadata
    for (auto i = begin; i != end; ++i) {
        size += i->data.size();
    }
    return (int)size;
}

void fcMP4Context::eraseFrame(int begin_frame, int end_frame)
{
    wait();
    scrape(false);

    adjust_frame(begin_frame, end_frame, (int)m_h264_buffers.size());
    auto begin = m_h264_buffers.begin();
    auto end = m_h264_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);
    m_h264_buffers.erase(begin, end);
    // todo: remake IDR frame if needed
}


fcCLinkage fcExport fcIMP4Context* fcMP4CreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice *dev)
{
    if (fcH264Encoder::loadModule()) {
        return new fcMP4Context(conf, dev);
    }
    return nullptr;
}


#endif // fcSupportMP4
