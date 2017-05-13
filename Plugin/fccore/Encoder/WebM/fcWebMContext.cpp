#include "pch.h"
#include "fcInternal.h"

#ifdef fcSupportWebM
#include "fcWebMInternal.h"
#include "fcWebMContext.h"
#include "fcWebMWriter.h"
#include "fcVorbisEncoder.h"
#include "fcVPXEncoder.h"


class fcWebMContext : public fcIWebMContext
{
public:
    using VideoEncoderPtr   = std::unique_ptr<fcIWebMVideoEncoder>;
    using AudioEncoderPtr   = std::unique_ptr<fcIWebMAudioEncoder>;
    using WriterPtr         = std::unique_ptr<fcIWebMWriter>;
    using WriterPtrs        = std::vector<WriterPtr>;

    using VideoBuffer       = Buffer;
    using VideoBuffers      = SharedResources<VideoBuffer>;

    using AudioBuffer       = RawVector<float>;
    using AudioBuffers      = SharedResources<AudioBuffer>;


    fcWebMContext(fcWebMConfig &conf, fcIGraphicsDevice *gd);
    ~fcWebMContext() override;

    void addOutputStream(fcStream *s) override;

    bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamp);
    void flushVideo();

    bool addAudioSamples(const float *samples, int num_samples) override;
    void flushAudio();


    // Body: [](fcIWebMWriter& writer) {}
    template<class Body>
    void eachStreams(const Body &b)
    {
        for (auto& s : m_writers) { b(*s); }
    }

private:
    fcWebMConfig        m_conf;
    fcIGraphicsDevice   *m_gdev = nullptr;

    WriterPtrs          m_writers;

    TaskQueue           m_video_tasks;
    VideoEncoderPtr     m_video_encoder;
    VideoBuffers        m_video_buffers;
    fcWebMFrameData     m_video_frame;

    TaskQueue           m_audio_tasks;
    AudioEncoderPtr     m_audio_encoder;
    AudioBuffers        m_audio_buffers;
    fcWebMFrameData     m_audio_frame;
};


fcWebMContext::fcWebMContext(fcWebMConfig &conf, fcIGraphicsDevice *gd)
    : m_conf(conf)
    , m_gdev(gd)
{
    m_conf.video_max_tasks = std::max<int>(m_conf.video_max_tasks, 1);
    m_conf.audio_max_tasks = std::max<int>(m_conf.audio_max_tasks, 1);

    if (conf.video) {
        fcVPXEncoderConfig econf;
        econf.width = conf.video_width;
        econf.height = conf.video_height;
        econf.target_framerate = conf.video_target_framerate;
        econf.bitrate_mode = conf.video_bitrate_mode;
        econf.target_bitrate = conf.video_target_bitrate;

        switch (conf.video_encoder) {
        case fcWebMVideoEncoder::VP8:
            m_video_encoder.reset(fcCreateVP8EncoderVPX(econf));
            break;
        case fcWebMVideoEncoder::VP9:
            m_video_encoder.reset(fcCreateVP9EncoderVPX(econf));
            break;
        case fcWebMVideoEncoder::VP9LossLess:
            m_video_encoder.reset(fcCreateVP9LossLessEncoderVPX(econf));
            break;
        }

        for (int i = 0; i < m_conf.video_max_tasks; ++i) {
            m_video_buffers.emplace();
        }
    }

    if (conf.audio) {
        fcVorbisEncoderConfig econf;
        econf.sample_rate = conf.audio_sample_rate;
        econf.num_channels = conf.audio_num_channels;
        econf.bitrate_mode = conf.audio_bitrate_mode;
        econf.target_bitrate = conf.audio_target_bitrate;

        switch (conf.audio_encoder) {
        case fcWebMAudioEncoder::Vorbis:
            m_audio_encoder.reset(fcCreateVorbisEncoder(econf));
            break;
        case fcWebMAudioEncoder::Opus:
            m_audio_encoder.reset(fcCreateOpusEncoder(econf));
            break;
        }

        for (int i = 0; i < m_conf.audio_max_tasks; ++i) {
            m_audio_buffers.emplace();
        }
    }
}

fcWebMContext::~fcWebMContext()
{
    flushVideo();
    flushAudio();
    m_writers.clear();
    m_video_encoder.reset();
    m_audio_encoder.reset();
}

void fcWebMContext::addOutputStream(fcStream *s)
{
    if (!s) { return; }

    auto *writer = fcCreateWebMWriter(s, m_conf);
    if (m_video_encoder) { writer->setVideoEncoderInfo(*m_video_encoder); }
    if (m_audio_encoder) { writer->setAudioEncoderInfo(*m_audio_encoder); }
    m_writers.emplace_back(writer);
}


bool fcWebMContext::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!tex || !m_video_encoder || !m_gdev) { return false; }

    auto buf = m_video_buffers.acquire();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    if (m_gdev->readTexture(buf->data(), buf->size(), tex, m_conf.video_width, m_conf.video_height, fmt)) {
        m_video_tasks.run([this, buf, fmt, timestamp]() {
            addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
        });
    }
    else {
        return false;
    }
    return true;
}

bool fcWebMContext::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!pixels || !m_video_encoder) { return false; }

    auto buf = m_video_buffers.acquire();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    memcpy(buf->data(), pixels, size);

    m_video_tasks.run([this, buf, fmt, timestamp]() {
        addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
    });
    return true;
}

bool fcWebMContext::addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    // encode!
    if (m_video_encoder->encode(m_video_frame, pixels, fmt, timestamp)) {
        eachStreams([&](fcIWebMWriter& writer) {
            writer.addVideoFrame(m_video_frame);
        });
        m_video_frame.clear();
    }

    return true;
}

void fcWebMContext::flushVideo()
{
    if (!m_video_encoder) { return; }

    m_video_tasks.run([this]() {
        if (m_video_encoder->flush(m_video_frame)) {
            eachStreams([&](fcIWebMWriter& writer) {
                writer.addVideoFrame(m_video_frame);
            });
            m_video_frame.clear();
        }
    });
    m_video_tasks.wait();
}


bool fcWebMContext::addAudioSamples(const float *samples, int num_samples)
{
    if (!samples || !m_audio_encoder) { return false; }

    auto buf = m_audio_buffers.acquire();
    buf->assign(samples, num_samples);

    m_audio_tasks.run([this, buf]() {
        if (m_audio_encoder->encode(m_audio_frame, buf->data(), buf->size())) {
            eachStreams([&](fcIWebMWriter& writer) {
                writer.AddAudioSamples(m_audio_frame);
            });
            m_audio_frame.clear();
        }
    });
    return true;
}

void fcWebMContext::flushAudio()
{
    if (!m_audio_encoder) { return; }

    m_audio_tasks.run([this]() {
        if (m_audio_encoder->flush(m_audio_frame)) {
            eachStreams([&](fcIWebMWriter& writer) {
                writer.AddAudioSamples(m_audio_frame);
            });
            m_audio_frame.clear();
        }
    });
    m_audio_tasks.wait();
}


fcIWebMContext* fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice *gd) { return new fcWebMContext(conf, gd); }

#else  // fcSupportWebM

fcIWebMContext* fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice *gd) { return nullptr; }

#endif // fcSupportWebM
