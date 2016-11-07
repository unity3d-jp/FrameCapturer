#include "pch.h"
#include <libyuv/libyuv.h>
#include "fcThreadPool.h"
#include "fcMP4Internal.h"
#include "fcMP4Context.h"
#include "fcH264Encoder.h"
#include "fcAACEncoder.h"
#include "fcMP4Writer.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "Foundation/TaskQueue.h"

#define fcMP4DefaultMaxBuffers 4


class fcMP4Context : public fcIMP4Context
{
public:
    using VideoEncoderPtr   = std::unique_ptr<fcIH264Encoder>;
    using AudioEncoderPtr   = std::unique_ptr<fcIAACEncoder>;
    using WriterPtr         = std::unique_ptr<fcMP4Writer>;
    using WriterPtrs        = std::vector<WriterPtr>;

    using VideoBuffer       = Buffer;
    using VideoBufferPtr    = std::shared_ptr<VideoBuffer>;
    using VideoBufferQueue  = ResourceQueue<VideoBufferPtr>;

    using AudioBuffer       = RawVector<float>;
    using AudioBufferPtr    = std::shared_ptr<AudioBuffer>;
    using AudioBufferQueue  = ResourceQueue<AudioBufferPtr>;


    fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev);
    ~fcMP4Context();
    void release() override;

    const char* getAudioEncoderInfo() override;
    const char* getVideoEncoderInfo() override;

    void addOutputStream(fcStream *s) override;
    bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamps) override;
    bool addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamps);

    bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp) override;
    bool addAudioFrameImpl(const float *samples, int num_samples, fcTime timestamp);

private:
    template<class Body>
    void eachStreams(const Body &b)
    {
        for (auto& s : m_writers) { b(*s); }
    }

private:
    fcMP4Config m_conf;
    fcIGraphicsDevice *m_dev;

    WriterPtrs          m_writers;

    TaskQueue           m_video_tasks;
    VideoEncoderPtr     m_video_encoder;
    VideoBufferQueue    m_video_buffers;
    Buffer              m_rgba_image;
    I420Image           m_i420_image;
    fcH264Frame         m_video_frame;

    TaskQueue           m_audio_tasks;
    AudioEncoderPtr     m_audio_encoder;
    AudioBufferQueue    m_audio_buffers;
    fcAACFrame          m_audio_frame;

#ifndef fcMaster
    std::unique_ptr<StdIOStream> m_dbg_h264_out;
    std::unique_ptr<StdIOStream> m_dbg_aac_out;
#endif // fcMaster
};



fcMP4Context::fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev)
    : m_conf(conf)
    , m_dev(dev)
{
#ifndef fcMaster
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


    // create h264 encoder
    m_video_encoder.reset();
    if (m_conf.video) {
        fcH264EncoderConfig h264conf;
        h264conf.width = m_conf.video_width;
        h264conf.height = m_conf.video_height;
        h264conf.target_framerate = m_conf.video_target_framerate;
        h264conf.bitrate_mode = m_conf.video_bitrate_mode;
        h264conf.target_bitrate = m_conf.video_target_bitrate;

        fcIH264Encoder *enc = nullptr;
        if (!enc && (m_conf.video_flags & fcMP4_H264NVIDIA) != 0) {
            enc = fcCreateH264EncoderNVIDIA(h264conf);
        }
        if (!enc && (m_conf.video_flags & fcMP4_H264AMD) != 0) {
            enc = fcCreateH264EncoderAMD(h264conf);
        }
        if (!enc && (m_conf.video_flags & (fcMP4_H264IntelHW | fcMP4_H264IntelSW)) != 0) {
            enc = fcCreateH264EncoderIntel(h264conf);
        }
        if (!enc && (m_conf.video_flags & fcMP4_H264OpenH264) != 0) {
            enc = fcCreateH264EncoderOpenH264(h264conf);
        }

        if (enc) {
            m_video_encoder.reset(enc);
            for (int i = 0; i < 4; ++i) {
                m_video_buffers.push(VideoBufferPtr(new VideoBuffer()));
            }
            m_video_tasks.start();
        }
    }

    // create aac encoder
    m_audio_encoder.reset();
    if (m_conf.audio) {
        fcAACEncoderConfig aacconf;
        aacconf.sample_rate = m_conf.audio_sample_rate;
        aacconf.num_channels = m_conf.audio_num_channels;
        aacconf.bitrate_mode = m_conf.audio_bitrate_mode;
        aacconf.target_bitrate = m_conf.audio_target_bitrate;

        fcIAACEncoder *enc = nullptr;
        if (!enc && (m_conf.audio_flags & fcMP4_AACIntel) != 0) {
            enc = fcCreateAACEncoderIntel(aacconf);
        }
        if (!enc && (m_conf.video_flags & fcMP4_AACFAAC) != 0) {
            enc = fcCreateAACEncoderFAAC(aacconf);
        }

        if (enc) {
            m_audio_encoder.reset(enc);
            for (int i = 0; i < 4; ++i) {
                m_audio_buffers.push(AudioBufferPtr(new AudioBuffer()));
            }
            m_audio_tasks.start();
        }
    }
}

fcMP4Context::~fcMP4Context()
{
    if (m_conf.video) { m_video_tasks.stop(); }
    if (m_conf.audio) { m_audio_tasks.stop(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    m_video_encoder.reset();
    m_audio_encoder.reset();
    m_writers.clear();

#ifndef fcMaster
    m_dbg_h264_out.reset();
    m_dbg_aac_out.reset();
#endif // fcMaster
}


void fcMP4Context::release()
{
    delete this;
}

const char* fcMP4Context::getAudioEncoderInfo()
{
    if (!m_audio_encoder) { return ""; }
    return m_audio_encoder->getEncoderInfo();
}

const char* fcMP4Context::getVideoEncoderInfo()
{
    if (!m_video_encoder) { return ""; }
    return m_video_encoder->getEncoderInfo();
}

void fcMP4Context::addOutputStream(fcStream *s)
{
    auto writer = new fcMP4Writer(*s, m_conf);
    if (m_audio_encoder) {
        writer->setAACEncoderInfo(m_audio_encoder->getDecoderSpecificInfo());
    }
    m_writers.emplace_back(WriterPtr(writer));
}

bool fcMP4Context::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!tex || !m_video_encoder || !m_dev) { return false; }

    auto buf = m_video_buffers.pop();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    if (m_dev->readTexture(buf->data(), buf->size(), tex, m_conf.video_width, m_conf.video_height, fmt)) {
        m_video_tasks.run([this, buf, fmt, timestamp]() {
            addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
            m_video_buffers.push(buf);
        });
    }
    else {
        m_video_buffers.push(buf);
        return false;
    }
    return true;
}

bool fcMP4Context::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!pixels || !m_video_encoder) { return false; }

    auto buf = m_video_buffers.pop();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    memcpy(buf->data(), pixels, size);

    m_video_tasks.run([this, buf, fmt, timestamp]() {
        addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
        m_video_buffers.push(buf);
    });
    return true;

}

bool fcMP4Context::addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    I420Data i420;

    // convert image to I420
    if (fmt == fcPixelFormat_I420) {
        // use input data directly
        int frame_size = m_conf.video_width * m_conf.video_height;
        i420.y = (void*)pixels;
        i420.u = (char*)i420.y + frame_size;
        i420.v = (char*)i420.u + (frame_size >> 2);
    }
    else if (fmt == fcPixelFormat_RGBAu8) {
        // RGBAu8 -> I420
        m_i420_image.resize(m_conf.video_width, m_conf.video_height);
        RGBA2I420(m_i420_image, pixels, m_conf.video_width, m_conf.video_height);
        i420 = m_i420_image.data();
    }
    else {
        // any format -> RGBAu8 -> I420
        m_rgba_image.resize(m_conf.video_width * m_conf.video_height * 4);
        fcConvertPixelFormat(m_rgba_image.data(), fcPixelFormat_RGBAu8, pixels, fmt, m_conf.video_width * m_conf.video_height);

        m_i420_image.resize(m_conf.video_width, m_conf.video_height);
        RGBA2I420(m_i420_image, m_rgba_image.data(), m_conf.video_width, m_conf.video_height);
        i420 = m_i420_image.data();
    }

    // encode!
    if (m_video_encoder->encode(m_video_frame, i420, timestamp)) {
        eachStreams([this](auto& s) { s.addVideoFrame(m_video_frame); });
#ifndef fcMaster
        m_dbg_h264_out->write(m_video_frame.data.data(), m_video_frame.data.size());
#endif // fcMaster
        m_video_frame.clear();
        return true;
    }
    return false;
}

bool fcMP4Context::addAudioFrame(const float *samples, int num_samples, fcTime timestamp)
{
    if (!m_audio_encoder) {
        fcDebugLog("fcMP4Context::addAudioFrame(): aac encoder is null.");
        return false;
    }

    auto buf = m_audio_buffers.pop();
    buf->assign(samples, num_samples);

    m_audio_tasks.run([this, buf, timestamp]() {
        addAudioFrameImpl(buf->data(), (int)buf->size(), timestamp);
        m_audio_buffers.push(buf);
    });
    return true;
}

bool fcMP4Context::addAudioFrameImpl(const float *samples, int num_samples, fcTime timestamp)
{
    if (m_audio_encoder->encode(m_audio_frame, samples, num_samples, timestamp)) {
        eachStreams([this](auto& s) { s.addAudioFrame(m_audio_frame); });
#ifndef fcMaster
        m_dbg_aac_out->write(m_audio_frame.data.data(), m_audio_frame.data.size());
#endif // fcMaster
        m_audio_frame.clear();
        return true;
    }
    return false;
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

