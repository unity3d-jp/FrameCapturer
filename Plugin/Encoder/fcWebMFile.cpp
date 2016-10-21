#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcWebMFile.h"
#include "fcVorbisEncoder.h"
#include "fcVPXEncoder.h"
#include "fcWebMWriter.h"
#include "GraphicsDevice/fcGraphicsDevice.h"

#ifdef _MSC_VER
    #pragma comment(lib, "vpxmt.lib")
    #pragma comment(lib, "libwebm.lib")
    #pragma comment(lib, "libvorbis_static.lib")
    #pragma comment(lib, "libogg_static.lib")
    #pragma comment(lib, "opus.lib")
    #pragma comment(lib, "celt.lib")
    #pragma comment(lib, "silk_common.lib")
    #pragma comment(lib, "silk_float.lib")
#endif // _MSC_VER



class fcWebMContext : public fcIWebMContext
{
public:
    fcWebMContext(fcWebMConfig &conf, fcIGraphicsDevice *gd);
    ~fcWebMContext() override;
    void release() override;

    void addOutputStream(fcStream *s) override;

    bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp) override;
    void flushVideo();

    bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp) override;
    void flushAudio();


    // Body: [](fcIWebMWriter& writer) {}
    template<class Body>
    void eachStreams(const Body &b)
    {
        for (auto& s : m_writers) { b(*s); }
    }

private:
    using VideoEncoderPtr = std::unique_ptr<fcIWebMVideoEncoder>;
    using AudioEncoderPtr = std::unique_ptr<fcIWebMAudioEncoder>;
    using WriterPtr       = std::unique_ptr<fcIWebMWriter>;
    using WriterPtrs      = std::vector<WriterPtr>;

    fcWebMConfig        m_conf;
    fcIGraphicsDevice   *m_gdev = nullptr;
    VideoEncoderPtr     m_video_encoder;
    AudioEncoderPtr     m_audio_encoder;
    WriterPtrs          m_writers;

    Buffer              m_texture_image;
    Buffer              m_rgba_image;
    fcI420Image         m_i420_image;
    RawVector<float>    m_audio_samples;
    fcWebMVideoFrame    m_video_frame;
    fcWebMAudioFrame    m_audio_frame;
};


fcWebMContext::fcWebMContext(fcWebMConfig &conf, fcIGraphicsDevice *gd)
    : m_conf(conf)
    , m_gdev(gd)
{
    if (conf.video) {
        fcVPXEncoderConfig econf;
        econf.width = conf.video_width;
        econf.height = conf.video_height;
        econf.target_bitrate = conf.video_bitrate;

        switch (conf.video_encoder) {
        case fcWebMVideoEncoder::VP8:
            m_video_encoder.reset(fcCreateVP8Encoder(econf));
            break;
        case fcWebMVideoEncoder::VP9:
            m_video_encoder.reset(fcCreateVP9Encoder(econf));
            break;
        }
    }

    if (conf.audio) {
        fcVorbisEncoderConfig econf;
        econf.sample_rate = conf.audio_sample_rate;
        econf.num_channels = conf.audio_num_channels;
        econf.target_bitrate = conf.audio_bitrate;

        switch (conf.audio_encoder) {
        case fcWebMAudioEncoder::Vorbis:
            m_audio_encoder.reset(fcCreateVorbisEncoder(econf));
            break;
        case fcWebMAudioEncoder::Opus:
            m_audio_encoder.reset(fcCreateOpusEncoder(econf));
            break;
        }
    }
}

fcWebMContext::~fcWebMContext()
{
    flushVideo();
    flushAudio();

    m_video_encoder.reset();
    m_audio_encoder.reset();
    m_writers.clear();
}

void fcWebMContext::release()
{
    delete this;
}

void fcWebMContext::addOutputStream(fcStream *s)
{
    auto *writer = fcCreateWebMMuxer(*s, m_conf);
    if (m_video_encoder) { writer->setVideoEncoderInfo(*m_video_encoder); }
    if (m_audio_encoder) { writer->setAudioEncoderInfo(*m_audio_encoder); }
    m_writers.emplace_back(writer);
}

bool fcWebMContext::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!tex || !m_video_encoder || !m_gdev) { return false; }

    size_t psize = fcGetPixelSize(fmt);
    m_texture_image.resize(m_conf.video_width * m_conf.video_height * psize);
    if (!m_gdev->readTexture(m_texture_image.data(), m_texture_image.size(), tex, m_conf.video_width, m_conf.video_height, fmt))
    {
        return false;
    }

    addVideoFramePixels(m_texture_image.data(), fmt, timestamp);
    return true;
}

bool fcWebMContext::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!pixels || !m_video_encoder) { return false; }

    fcI420Data i420;
    if (fmt == fcPixelFormat_I420) {
        int frame_size = m_conf.video_width * m_conf.video_height;
        i420.y = pixels;
        i420.u = (char*)i420.y + frame_size;
        i420.v = (char*)i420.u + (frame_size >> 2);
    }
    else if (fmt == fcPixelFormat_RGBAu8) {
        m_i420_image.resize(m_conf.video_width, m_conf.video_height);
        fcRGBA2I420(m_i420_image, pixels, m_conf.video_width, m_conf.video_height);
        i420 = m_i420_image.data();
    }
    else {
        m_rgba_image.resize(m_conf.video_width * m_conf.video_height * 4);
        fcConvertPixelFormat(m_rgba_image.data(), fcPixelFormat_RGBAu8, pixels, fmt, m_conf.video_width * m_conf.video_height);

        m_i420_image.resize(m_conf.video_width, m_conf.video_height);
        fcRGBA2I420(m_i420_image, m_rgba_image.data(), m_conf.video_width, m_conf.video_height);
        i420 = m_i420_image.data();
    }

    if (m_video_encoder->encode(m_video_frame, i420, timestamp)) {
        eachStreams([&](auto& writer) {
            writer.addVideoFrame(m_video_frame);
        });
        m_video_frame.clear();
    }
    return true;
}

void fcWebMContext::flushVideo()
{
    if (!m_video_encoder) { return; }

    if (m_video_encoder->flush(m_video_frame)) {
        eachStreams([&](auto& writer) {
            writer.addVideoFrame(m_video_frame);
        });
        m_video_frame.clear();
    }
}


bool fcWebMContext::addAudioFrame(const float *samples, int num_samples, fcTime timestamp)
{
    if (!samples || !m_audio_encoder) { return false; }

    m_audio_samples.assign(samples, num_samples);

    if (m_audio_encoder->encode(m_audio_frame, m_audio_samples.data(), m_audio_samples.size())) {
        eachStreams([&](auto& writer) {
            writer.addAudioFrame(m_audio_frame);
        });
        m_audio_frame.clear();
    }
    return true;
}

void fcWebMContext::flushAudio()
{
    if (!m_audio_encoder) { return; }

    if (m_audio_encoder->flush(m_audio_frame)) {
        eachStreams([&](auto& writer) {
            writer.addAudioFrame(m_audio_frame);
        });
        m_audio_frame.clear();
    }
}


fcWebMAPI fcIWebMContext* fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice *gd)
{
    return new fcWebMContext(conf, gd);
}