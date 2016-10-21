#include "pch.h"
#include <libyuv/libyuv.h>
#include "fcThreadPool.h"
#include "fcMP4Internal.h"
#include "fcMP4File.h"
#include "fcH264Encoder.h"
#include "fcAACEncoder.h"
#include "fcMP4StreamWriter.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#ifdef fcWindows
    #pragma comment(lib, "yuv.lib")
#endif

#define fcMP4DefaultMaxBuffers 4


class fcMP4Context : public fcIMP4Context
{
public:
    fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev);
    ~fcMP4Context();
    void release() override;

    const char* getAudioEncoderInfo() override;
    const char* getVideoEncoderInfo() override;

    void addOutputStream(fcStream *s) override;
    bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamps) override;
    bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp) override;

private:
    typedef std::pair<fcVideoFrame, fcH264Frame> VideoFrame;
    typedef std::pair<fcAudioFrame, fcAACFrame> AudioFrame;
    typedef std::unique_ptr<fcMP4StreamWriter> StreamWriterPtr;

    void enqueueVideoTask(const std::function<void()> &f);
    void enqueueAudioTask(const std::function<void()> &f);
    void processVideoTasks();
    void processAudioTasks();

    VideoFrame& getTempraryVideoFrame();
    void        returnTempraryVideoFrame(VideoFrame& v);
    AudioFrame& getTempraryAudioFrame();
    void        returnTempraryAudioFrame(AudioFrame& v);

    void resetEncoders();
    void waitAllTasksFinished();
    void encodeVideoFrame(VideoFrame& vf, bool rgba2i420);

    template<class Body>
    void eachStreams(const Body &b)
    {
        for (auto& s : m_streams) { b(*s); }
    }

private:
    fcMP4Config m_conf;
    fcIGraphicsDevice *m_dev;
    bool m_stop;

    std::vector<VideoFrame>     m_tmp_video_frames;
    std::vector<AudioFrame>     m_tmp_audio_frames;
    std::vector<VideoFrame*>    m_tmp_video_frames_unused;
    std::vector<AudioFrame*>    m_tmp_audio_frames_unused;

    std::unique_ptr<fcIH264Encoder> m_h264_encoder;
    std::unique_ptr<fcIAACEncoder> m_aac_encoder;
    std::vector<StreamWriterPtr> m_streams;

    std::atomic_int m_video_active_task_count;
    std::thread m_video_worker;
    std::mutex m_video_mutex;
    std::condition_variable m_video_condition;
    std::deque<std::function<void()>> m_video_tasks;

    std::atomic_int m_audio_active_task_count;
    std::thread m_audio_worker;
    std::mutex m_audio_mutex;
    std::condition_variable m_audio_condition;
    std::deque<std::function<void()>> m_audio_tasks;

#ifndef fcMaster
    std::unique_ptr<StdIOStream> m_dbg_h264_out;
    std::unique_ptr<StdIOStream> m_dbg_aac_out;
#endif // fcMaster
};



fcMP4Context::fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev)
    : m_conf(conf)
    , m_dev(dev)
    , m_stop(false)
    , m_video_active_task_count(0)
    , m_audio_active_task_count(0)
{
    if (m_conf.video_max_buffers == 0) {
        m_conf.video_max_buffers = fcMP4DefaultMaxBuffers;
    }

    // allocate temporary buffers and start encoder threads
    if (m_conf.video) {
        m_tmp_video_frames.resize(m_conf.video_max_buffers);
        for (auto& v : m_tmp_video_frames) {
            v.first.allocate(m_conf.video_width, m_conf.video_height);
            m_tmp_video_frames_unused.push_back(&v);
        }

        m_video_worker = std::thread([this]() { processVideoTasks(); });
    }
    if (m_conf.audio) {
        m_tmp_audio_frames.resize(m_conf.video_max_buffers);
        for (auto& v : m_tmp_audio_frames) {
            m_tmp_audio_frames_unused.push_back(&v);
        }

        m_audio_worker = std::thread([this]() { processAudioTasks(); });
    }

#ifndef fcMaster
    // output raw h264 & aac packets to file
    {
        uint64_t now = (uint64_t)::time(nullptr);
        char tmp_h264_filename[256];
        char tmp_aac_filename[256];
        sprintf(tmp_h264_filename, "%llu.h264", now);
        sprintf(tmp_aac_filename, "%llu.aac", now);

        m_dbg_h264_out.reset(new StdIOStream(new std::fstream(tmp_h264_filename, std::ios::binary | std::ios::out), true));
        m_dbg_aac_out.reset(new StdIOStream(new std::fstream(tmp_aac_filename, std::ios::binary | std::ios::out), true));
    }
#endif // fcMaster


    resetEncoders();
}

fcMP4Context::~fcMP4Context()
{
    // stop encoder threads
    m_stop = true;
    if (m_conf.video) {
        m_video_condition.notify_all();
        m_video_worker.join();
    }
    if (m_conf.audio) {
        m_audio_condition.notify_all();
        m_audio_worker.join();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

#ifndef fcMaster
    m_dbg_h264_out.reset();
    m_dbg_aac_out.reset();
#endif // fcMaster

    m_streams.clear();
}

void fcMP4Context::resetEncoders()
{
    waitAllTasksFinished();

    // create h264 encoder
    m_h264_encoder.reset();
    if (m_conf.video) {
        fcH264EncoderConfig h264conf;
        h264conf.width = m_conf.video_width;
        h264conf.height = m_conf.video_height;
        h264conf.max_framerate = m_conf.video_max_framerate;
        h264conf.target_bitrate = m_conf.video_bitrate;

        fcIH264Encoder *enc = nullptr;
        // try to create hardware encoder
        if (m_conf.video_use_hardware_encoder_if_possible) {
            enc = fcCreateNVH264Encoder(h264conf);
            if (enc == nullptr) {
                enc = fcCreateAMDH264Encoder(h264conf);
            }
        }
        if (enc == nullptr) {
            // fallback to software encoder (OpenH264)
            enc = fcCreateOpenH264Encoder(h264conf);
        }
        m_h264_encoder.reset(enc);
    }

    // create aac encoder
    m_aac_encoder.reset();
    if (m_conf.audio) {
        fcAACEncoderConfig aacconf;
        aacconf.sample_rate = m_conf.audio_sample_rate;
        aacconf.num_channels = m_conf.audio_num_channels;
        aacconf.target_bitrate = m_conf.audio_bitrate;

        m_aac_encoder.reset(fcCreateFAACEncoder(aacconf));
    }
}

void fcMP4Context::enqueueVideoTask(const std::function<void()> &f)
{
    {
        std::unique_lock<std::mutex> lock(m_video_mutex);
        m_video_tasks.push_back(f);
    }
    m_video_condition.notify_one();
}

void fcMP4Context::enqueueAudioTask(const std::function<void()> &f)
{
    {
        std::unique_lock<std::mutex> lock(m_audio_mutex);
        m_audio_tasks.push_back(f);
    }
    m_audio_condition.notify_one();
}

void fcMP4Context::waitAllTasksFinished()
{
    while (m_video_active_task_count > 0 || m_audio_active_task_count > 0) {
        std::this_thread::yield();
    }
}


fcMP4Context::VideoFrame& fcMP4Context::getTempraryVideoFrame()
{
    VideoFrame *ret = nullptr;

    // wait if all temporaries are in use
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(m_video_mutex);
            if (!m_tmp_video_frames_unused.empty()) {
                ret = m_tmp_video_frames_unused.back();
                m_tmp_video_frames_unused.pop_back();
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return *ret;
}

void fcMP4Context::returnTempraryVideoFrame(VideoFrame& v)
{
    std::unique_lock<std::mutex> lock(m_video_mutex);
    m_tmp_video_frames_unused.push_back(&v);
}

fcMP4Context::AudioFrame& fcMP4Context::getTempraryAudioFrame()
{
    AudioFrame *ret = nullptr;

    // wait if all temporaries are in use
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(m_audio_mutex);
            if (!m_tmp_audio_frames_unused.empty()) {
                ret = m_tmp_audio_frames_unused.back();
                m_tmp_audio_frames_unused.pop_back();
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return *ret;
}

void fcMP4Context::returnTempraryAudioFrame(AudioFrame& v)
{
    std::unique_lock<std::mutex> lock(m_audio_mutex);
    m_tmp_audio_frames_unused.push_back(&v);
}


void fcMP4Context::processVideoTasks()
{
    while (!m_stop)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_video_mutex);
            while (!m_stop && m_video_tasks.empty()) {
                m_video_condition.wait(lock);
            }
            if (m_stop) { return; }

            task = m_video_tasks.front();
            m_video_tasks.pop_front();
        }
        task();
    }
}

void fcMP4Context::processAudioTasks()
{
    while (!m_stop)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_audio_mutex);
            while (!m_stop && m_audio_tasks.empty()) {
                m_audio_condition.wait(lock);
            }
            if (m_stop) { return; }

            task = m_audio_tasks.front();
            m_audio_tasks.pop_front();
        }
        task();
    }
}


void fcMP4Context::release()
{
    delete this;
}

const char* fcMP4Context::getAudioEncoderInfo()
{
    if (!m_aac_encoder) { return ""; }
    return m_aac_encoder->getEncoderInfo();
}

const char* fcMP4Context::getVideoEncoderInfo()
{
    if (!m_h264_encoder) { return ""; }
    return m_h264_encoder->getEncoderInfo();
}

void fcMP4Context::addOutputStream(fcStream *s)
{
    auto writer = new fcMP4StreamWriter(*s, m_conf);
    if (m_aac_encoder) {
        writer->setAACEncoderInfo(m_aac_encoder->getDecoderSpecificInfo());
    }
    m_streams.emplace_back(StreamWriterPtr(writer));
}

void fcMP4Context::encodeVideoFrame(VideoFrame& vf, bool rgba2i420)
{
    auto& raw = vf.first;
    auto& h264 = vf.second;

    // 必要であれば RGBA -> I420 変換
    int width = m_conf.video_width;
    int frame_size = m_conf.video_width * m_conf.video_height;
    uint8 *y = (uint8*)raw.i420.y;
    uint8 *u = (uint8*)raw.i420.u;
    uint8 *v = (uint8*)raw.i420.v;
    if (rgba2i420) {
        libyuv::ABGRToI420(
            (uint8*)&raw.rgba[0], width * 4,
            y, width,
            u, width >> 1,
            v, width >> 1,
            m_conf.video_width, m_conf.video_height );
    }

    // I420 のピクセルデータを H264 へエンコード
    h264.clear();
    h264.timestamp = raw.timestamp;
    m_h264_encoder->encode(h264, raw.i420, raw.timestamp);

    eachStreams([&](auto& s) { s.addFrame(h264); });
#ifndef fcMaster
    m_dbg_h264_out->write(h264.data.data(), h264.data.size());
#endif // fcMaster
}


bool fcMP4Context::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (m_dev == nullptr) {
        fcDebugLog("fcMP4Context::addVideoFrameTexture(): gfx device is null.");
        return false;
    }
    if (m_h264_encoder == nullptr) {
        fcDebugLog("fcMP4Context::addVideoFrameTexture(): h264 encoder is null.");
        return false;
    }

    VideoFrame& vf = getTempraryVideoFrame();
    auto& raw = vf.first;
    auto& h264 = vf.second;
    raw.timestamp = timestamp >= 0.0 ? timestamp : GetCurrentTimeSec();

    // フレームバッファの内容取得
    if (fmt == fcPixelFormat_RGBAu8) {
        if (!m_dev->readTexture(&raw.rgba[0], raw.rgba.size(), tex, m_conf.video_width, m_conf.video_height, fmt))
        {
            returnTempraryVideoFrame(vf);
            return false;
        }
    }
    else {
        size_t psize = fcGetPixelSize(fmt);
        raw.raw.resize(m_conf.video_width * m_conf.video_height * psize);
        if (!m_dev->readTexture(&raw.raw[0], raw.raw.size(), tex, m_conf.video_width, m_conf.video_height, fmt))
        {
            returnTempraryVideoFrame(vf);
            return false;
        }
        fcConvertPixelFormat(raw.rgba.data(), fcPixelFormat_RGBAu8, &raw.raw[0], fmt, m_conf.video_width * m_conf.video_height);
    }

    // h264 データを生成
    ++m_video_active_task_count;
    enqueueVideoTask([this, &vf](){
        encodeVideoFrame(vf, true);
        returnTempraryVideoFrame(vf);
        --m_video_active_task_count;
    });

    return true;
}

bool fcMP4Context::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (m_h264_encoder == nullptr) {
        fcDebugLog("fcMP4Context::addVideoFramePixels(): h264 encoder is null.");
        return false;
    }

    VideoFrame& vf = getTempraryVideoFrame();
    auto& raw = vf.first;
    auto& h264 = vf.second;
    raw.timestamp = timestamp >= 0.0 ? timestamp : GetCurrentTimeSec();

    bool rgba2i420 = true;
    if (fmt == fcPixelFormat_I420) {
        rgba2i420 = false;

        int frame_size = m_conf.video_width * m_conf.video_height;
        const uint8_t *src_y = (const uint8_t*)pixels;
        const uint8_t *src_u = src_y + frame_size;
        const uint8_t *src_v = src_u + (frame_size >> 2);
        memcpy(raw.i420.y, src_y, frame_size);
        memcpy(raw.i420.u, src_u, frame_size >> 2);
        memcpy(raw.i420.v, src_v, frame_size >> 2);
    }
    else if (fmt == fcPixelFormat_RGBAu8) {
        memcpy(raw.rgba.data(), pixels, raw.rgba.size());
    }
    else {
        fcConvertPixelFormat(raw.rgba.data(), fcPixelFormat_RGBAu8, pixels, fmt, m_conf.video_width * m_conf.video_height);
    }

    // h264 データを生成
    ++m_video_active_task_count;
    enqueueVideoTask([this, &vf, rgba2i420](){
        encodeVideoFrame(vf, rgba2i420);
        returnTempraryVideoFrame(vf);
        --m_video_active_task_count;
    });

    return true;
}

bool fcMP4Context::addAudioFrame(const float *samples, int num_samples, fcTime timestamp)
{
    if (m_aac_encoder == nullptr) {
        fcDebugLog("fcMP4Context::addAudioFrame(): aac encoder is null.");
        return false;
    }

    AudioFrame& af = getTempraryAudioFrame();
    auto& raw = af.first;
    auto& aac = af.second;
    raw.timestamp = timestamp >= 0.0 ? timestamp : GetCurrentTimeSec();
    raw.data = Buffer((char*)samples, sizeof(float)*num_samples);

    // aac encode
    ++m_audio_active_task_count;
    enqueueAudioTask([this, &aac, &raw, &af](){
        aac.clear();
        aac.timestamp = raw.timestamp;

        // apply audio_scale
        if (m_conf.audio_scale != 1.0f) {
            float *samples = (float*)raw.data.data();
            size_t num_samples = raw.data.size() / sizeof(float);
            fcScaleArray(samples, num_samples, m_conf.audio_scale);
        }

        m_aac_encoder->encode(aac, (float*)raw.data.data(), raw.data.size() / sizeof(float));

        eachStreams([&](auto& s) { s.addFrame(aac); });
#ifndef fcMaster
        m_dbg_aac_out->write(aac.data.data(), aac.data.size());
#endif // fcMaster

        returnTempraryAudioFrame(af);
        --m_audio_active_task_count;
    });

    return true;
}


namespace {
    std::string g_module_path;
    std::string g_faac_package_path;
    fcDownloadState g_openh264_download_state = fcDownloadState_Idle;
    fcDownloadState g_faac_download_state = fcDownloadState_Idle;
}

const std::string& fcMP4GetModulePath() { return g_module_path; }
const std::string& fcMP4GetFAACPackagePath() { return g_faac_package_path; }

fcMP4API fcIMP4Context* fcMP4CreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice *dev)
{
    if (fcLoadOpenH264Module()) {
        fcLoadFAACModule();
        return new fcMP4Context(conf, dev);
    }
    return nullptr;
}

fcMP4API void fcMP4SetModulePathImpl(const char *path)
{
    g_module_path = path;
}

fcMP4API void fcMP4SetFAACPackagePathImpl(const char *path)
{
    g_faac_package_path = path;
}

fcMP4API bool fcMP4DownloadCodecBeginImpl()
{
    if (fcLoadOpenH264Module() && fcLoadFAACModule()) {
        g_openh264_download_state = fcDownloadState_Completed;
        return true;
    }

    g_openh264_download_state = fcDownloadState_InProgress;
    bool openh264 = fcDownloadOpenH264([&](fcDownloadState state, const char *message) {
        fcDebugLog("fcDownloadOpenH264: %s\n", message);
        g_openh264_download_state = state;
    });

    bool faac = false;
    if (!fcMP4GetFAACPackagePath().empty()) {
        g_faac_download_state = fcDownloadState_InProgress;
        faac = fcDownloadFAAC([&](fcDownloadState state, const char *message) {
            fcDebugLog("fcDownloadFAAC: %s\n", message);
            g_faac_download_state = state;
        });
    }
    return openh264 || faac;
}

fcMP4API fcDownloadState fcMP4DownloadCodecGetStateImpl()
{
    return std::max<fcDownloadState>(g_openh264_download_state, g_faac_download_state);
}

