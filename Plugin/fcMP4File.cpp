#include "pch.h"

#ifdef fcSupportMP4
#include "FrameCapturer.h"
#include "fcThreadPool.h"
#include "fcGraphicsDevice.h"
#include "fcMP4File.h"

struct fcMP4FrameData
{

};


class fcMP4Context : public fcIMP4Context
{
public:
    fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev);
    ~fcMP4Context();
    void release() override;

    bool addFrame(void *tex) override;
    void clearFrame() override;
    bool writeFile(const char *path, int begin_frame, int end_frame) override;
    int  writeMemory(void *buf, int begin_frame, int end_frame) override;

    int getFrameCount() override;
    void getFrameData(void *tex, int frame) override;
    int getExpectedDataSize(int begin_frame, int end_frame) override;
    void eraseFrame(int begin_frame, int end_frame) override;

private:
    void scrape(bool is_tasks_running);
    void addFrameTask(fcMP4FrameData &o_fdata, const std::string &raw_buffer);
    void write(std::ostream &os, int begin_frame, int end_frame);

private:
    fcEMagic m_magic; //  for debug
    fcMP4Config m_conf;
    fcIGraphicsDevice *m_dev;
    std::vector<std::string> m_raw_buffers;
    std::list<fcMP4FrameData> m_mp4_buffers;
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
        rf.resize(m_conf.width * m_conf.height * fcGetPixelSize(fcE_ARGB32));
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

void fcMP4Context::addFrameTask(fcMP4FrameData &o_fdata, const std::string &raw_buffer)
{
    // todo
}

bool fcMP4Context::addFrame(void *tex)
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
    if (!m_dev->readTexture(&raw_buffer[0], raw_buffer.size(), tex, m_conf.width, m_conf.height, fcE_ARGB32))
    {
        --frame;
        return false;
    }

    // mp4 データを生成
    m_mp4_buffers.push_back(fcMP4FrameData());
    fcMP4FrameData& fdata = m_mp4_buffers.back();
    ++m_active_task_count;
    m_tasks.run([this, &fdata, &raw_buffer](){
        addFrameTask(fdata, raw_buffer);
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

    adjust_frame(begin_frame, end_frame, (int)m_mp4_buffers.size());
    auto begin = m_mp4_buffers.begin();
    auto end = m_mp4_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);

    // todo
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
    return m_mp4_buffers.size();
}

void fcMP4Context::getFrameData(void *tex, int frame)
{
    if (frame >= m_mp4_buffers.size()) { return; }
    m_tasks.wait();
    scrape(false);

    fcMP4FrameData *fdata;
    {
        auto it = m_mp4_buffers.begin();
        std::advance(it, frame);
        fdata = &(*it);
    }

    std::string raw_pixels;
    raw_pixels.resize(m_conf.width * m_conf.height * 4);
    // todo: decode
    m_dev->writeTexture(tex, m_conf.width, m_conf.height, fcE_ARGB32, &raw_pixels[0], raw_pixels.size());
}


int fcMP4Context::getExpectedDataSize(int begin_frame, int end_frame)
{
    adjust_frame(begin_frame, end_frame, (int)m_mp4_buffers.size());
    auto begin = m_mp4_buffers.begin();
    auto end = m_mp4_buffers.begin();
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

    adjust_frame(begin_frame, end_frame, (int)m_mp4_buffers.size());
    auto begin = m_mp4_buffers.begin();
    auto end = m_mp4_buffers.begin();
    std::advance(begin, begin_frame);
    std::advance(end, end_frame);
    m_mp4_buffers.erase(begin, end);
}


#endif // fcSupportMP4
