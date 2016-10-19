#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcWebMFile.h"
#include "fcVorbisEncoder.h"
#include "fcVPXEncoder.h"

#include "webm/mkvparser.hpp"
#include "webm/mkvreader.hpp"
#include "webm/mkvwriter.hpp"
#include "webm/mkvmuxer.hpp"
#include "webm/mkvmuxerutil.hpp"

#ifdef _MSC_VER
    #pragma comment(lib, "vpxmt.lib")
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

    bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp) override;

private:
    fcWebMConfig m_conf;
    fcIGraphicsDevice *m_gdev = nullptr;
    std::unique_ptr<fcIVPXEncoder> m_video_encoder;
    std::unique_ptr<fcIVorbisEncoder> m_audio_encoder;
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
            m_video_encoder.reset(fcCreateVP8Encoder(econf));
            break;
        }
    }

    if (conf.audio) {
        fcVorbisEncoderConfig econf;
        econf.sampling_rate = conf.audio_sample_rate;
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
}

void fcWebMContext::release()
{
    delete this;
}

void fcWebMContext::addOutputStream(fcStream *s)
{
    // todo
}

bool fcWebMContext::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!tex || !m_video_encoder || !m_gdev) { return false; }

    //m_video_encoder->encode(timestamp);
    return true;
}

bool fcWebMContext::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!pixels || !m_video_encoder) { return false; }

    return true;
}

bool fcWebMContext::addAudioFrame(const float *samples, int num_samples, fcTime timestamp)
{
    if (!samples || !m_audio_encoder) { return false; }

    return true;
}

fcWebMAPI fcIWebMContext* fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice *gd)
{
    return new fcWebMContext(conf, gd);
}