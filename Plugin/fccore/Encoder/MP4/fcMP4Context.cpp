#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcMP4Internal.h"
#include "fcMP4Context.h"
#include "fcH264Encoder.h"
#include "fcAACEncoder.h"
#include "fcMP4Writer.h"

#define fcMP4DefaultMaxBuffers 4


class fcMP4Context : public fcIMP4Context
{
public:
    using VideoEncoderPtr   = std::unique_ptr<fcIH264Encoder>;
    using AudioEncoderPtr   = std::unique_ptr<fcIAACEncoder>;
    using WriterPtr         = std::unique_ptr<fcMP4Writer>;
    using WriterPtrs        = std::vector<WriterPtr>;

    using VideoBuffer       = Buffer;
    using VideoBuffers      = SharedResources<VideoBuffer>;

    using AudioBuffer       = RawVector<float>;
    using AudioBuffers      = SharedResources<AudioBuffer>;


    fcMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev);
    ~fcMP4Context();
    bool isValid() const override;

    const char* getVideoEncoderInfo() override;
    const char* getAudioEncoderInfo() override;

    void addOutputStream(fcStream *s) override;
    bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamps) override;
    bool addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamps);
    void flushVideo();

    bool addAudioSamples(const float *samples, int num_samples) override;
    bool addAudioSamplesImpl(const float *samples, int num_samples);
    void flushAudio();

private:
    // Body: [](WriterPtr&) -> void
    template<class Body>
    void eachStreams(const Body &b)
    {
        for (auto& s : m_writers) { b(*s); }
    }

private:
    fcMP4Config         m_conf;
    fcIGraphicsDevice   *m_dev;

    WriterPtrs          m_writers;

    TaskQueue           m_video_tasks;
    VideoEncoderPtr     m_video_encoder;
    VideoBuffers        m_video_buffers;
    fcH264Frame         m_video_frame;

    TaskQueue           m_audio_tasks;
    AudioEncoderPtr     m_audio_encoder;
    AudioBuffers        m_audio_buffers;
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

        fcHWEncoderDeviceType hwdt = fcHWEncoderDeviceType::Unknown;
        if (m_dev) {
            switch (m_dev->getDeviceType()) {
            case fcGfxDeviceType::D3D9:  hwdt = fcHWEncoderDeviceType::D3D9; break;
            case fcGfxDeviceType::D3D10: hwdt = fcHWEncoderDeviceType::D3D10; break;
            case fcGfxDeviceType::D3D11: hwdt = fcHWEncoderDeviceType::D3D11; break;
            case fcGfxDeviceType::D3D12: hwdt = fcHWEncoderDeviceType::D3D12; break;
            case fcGfxDeviceType::CUDA: hwdt = fcHWEncoderDeviceType::CUDA; break;
            }
        }

        fcIH264Encoder *enc = nullptr;
        if (!enc && (m_conf.video_flags & fcMP4_H264NVIDIA) != 0) {
            // NVENC require D3D or CUDA device
            if (m_dev) {
                enc = fcCreateH264EncoderNVIDIA(h264conf, m_dev->getDevicePtr(), hwdt);
            }
        }
        if (!enc && (m_conf.video_flags & fcMP4_H264AMD) != 0) {
            if (m_dev) {
                enc = fcCreateH264EncoderAMD(h264conf, m_dev->getDevicePtr(), hwdt);
            }
        }
        if (!enc && (m_conf.video_flags & fcMP4_H264IntelHW) != 0) {
            if (m_dev) {
                enc = fcCreateH264EncoderIntelHW(h264conf, m_dev->getDevicePtr(), hwdt);
            }
        }
        if (!enc && (m_conf.video_flags & fcMP4_H264IntelSW) != 0) {
            enc = fcCreateH264EncoderIntelSW(h264conf);
        }
        if (!enc && (m_conf.video_flags & fcMP4_H264OpenH264) != 0) {
            enc = fcCreateH264EncoderOpenH264(h264conf);
        }

        if (enc) {
            m_video_encoder.reset(enc);
            for (int i = 0; i < 4; ++i) {
                m_video_buffers.push(new VideoBuffer());
            }
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
        if (!enc && (m_conf.audio_flags & fcMP4_AACFAAC) != 0) {
            enc = fcCreateAACEncoderFAAC(aacconf);
        }

        if (enc) {
            m_audio_encoder.reset(enc);
            for (int i = 0; i < 4; ++i) {
                m_audio_buffers.push(new AudioBuffer());
            }
        }
    }
}

fcMP4Context::~fcMP4Context()
{
    flushVideo();
    flushAudio();
    m_video_tasks.wait();
    m_audio_tasks.wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    m_video_encoder.reset();
    m_audio_encoder.reset();
    m_writers.clear();

#ifndef fcMaster
    m_dbg_h264_out.reset();
    m_dbg_aac_out.reset();
#endif // fcMaster
}


bool fcMP4Context::isValid() const
{
    return m_video_encoder || m_audio_encoder;
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
    auto writer = new fcMP4Writer(s, m_conf);
    if (m_audio_encoder) {
        writer->setAACEncoderInfo(m_audio_encoder->getDecoderSpecificInfo());
    }
    m_writers.emplace_back(WriterPtr(writer));
}

bool fcMP4Context::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!tex || !m_video_encoder || !m_dev) { return false; }

    auto buf = m_video_buffers.lock();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    if (m_dev->readTexture(buf->data(), buf->size(), tex, m_conf.video_width, m_conf.video_height, fmt)) {
        m_video_tasks.run([this, buf, fmt, timestamp]() {
            addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
            m_video_buffers.unlock(buf);
        });
    }
    else {
        m_video_buffers.unlock(buf);
        return false;
    }
    return true;
}

bool fcMP4Context::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!pixels || !m_video_encoder) { return false; }

    auto buf = m_video_buffers.lock();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    memcpy(buf->data(), pixels, size);

    m_video_tasks.run([this, buf, fmt, timestamp]() {
        addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
        m_video_buffers.unlock(buf);
    });
    return true;

}

bool fcMP4Context::addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    // encode!
    if (m_video_encoder->encode(m_video_frame, pixels, fmt, timestamp)) {
        eachStreams([this](fcMP4Writer& s) { s.addVideoFrame(m_video_frame); });
#ifndef fcMaster
        m_dbg_h264_out->write(m_video_frame.data.data(), m_video_frame.data.size());
#endif // fcMaster
        m_video_frame.clear();
        return true;
    }
    return false;
}

void fcMP4Context::flushVideo()
{
    if (!m_video_encoder) { return; }

    m_video_tasks.run([this]() {
        if (m_video_encoder->flush(m_video_frame)) {
            eachStreams([&](fcMP4Writer& writer) {
                writer.addVideoFrame(m_video_frame);
            });
            m_video_frame.clear();
        }
    });
}

bool fcMP4Context::addAudioSamples(const float *samples, int num_samples)
{
    if (!m_audio_encoder) {
        fcDebugLog("fcMP4Context::AddAudioSamples(): aac encoder is null.");
        return false;
    }

    auto buf = m_audio_buffers.lock();
    buf->assign(samples, num_samples);

    m_audio_tasks.run([this, buf]() {
        addAudioSamplesImpl(buf->data(), (int)buf->size());
        m_audio_buffers.unlock(buf);
    });
    return true;
}

bool fcMP4Context::addAudioSamplesImpl(const float *samples, int num_samples)
{
    if (m_audio_encoder->encode(m_audio_frame, samples, num_samples)) {
        eachStreams([this](fcMP4Writer& s) { s.AddAudioSamples(m_audio_frame); });
#ifndef fcMaster
        m_dbg_aac_out->write(m_audio_frame.data.data(), m_audio_frame.data.size());
#endif // fcMaster
        m_audio_frame.clear();
        return true;
    }
    return false;
}


void fcMP4Context::flushAudio()
{
    if (!m_audio_encoder) { return; }

    m_audio_tasks.run([this]() {
        if (m_audio_encoder->flush(m_audio_frame)) {
            eachStreams([&](fcMP4Writer& writer) {
                writer.AddAudioSamples(m_audio_frame);
            });
            m_audio_frame.clear();
        }
    });
}

namespace {
    std::string g_module_path;
    std::string g_faac_package_path;
}

const std::string& fcMP4GetModulePath() { return g_module_path; }
const std::string& fcMP4GetFAACPackagePath() { return g_faac_package_path; }

fcIMP4Context* fcMP4CreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice *dev)
{
    auto ret = new fcMP4Context(conf, dev);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

void fcMP4SetModulePathImpl(const char *path)
{
    g_module_path = path;
}
