#include "pch.h"
#include "fcFoundation.h"
#include "fcMP4Internal.h"
#include "fcAACEncoder.h"

#ifdef fcSupportAAC_Intel

#include "IntelQSV/mfxaudio++.h"
#ifdef _WIN32
    #pragma comment(lib, "libmfx.lib")
    #pragma comment(lib, "legacy_stdio_definitions.lib")
#endif


class fcAACEncoderIntel : public fcIAACEncoder
{
public:
    fcAACEncoderIntel(const fcAACEncoderConfig& conf);
    ~fcAACEncoderIntel() override;

    const char* getEncoderInfo() override;
    const Buffer& getDecoderSpecificInfo() override;
    bool encode(fcAACFrame& dst, const float *samples, size_t num_samples, fcTime timestamp) override;

    bool isValid() { return m_encoder != nullptr; }

private:
    fcAACEncoderConfig m_conf;
    void *m_handle;
    unsigned long m_num_read_samples;
    unsigned long m_output_size;
    Buffer m_aac_tmp_buf;
    Buffer m_aac_header;
    RawVector<float> m_tmp_data;

    MFXAudioSession m_session;
    std::unique_ptr<MFXAudioENCODE> m_encoder;
};


fcAACEncoderIntel::fcAACEncoderIntel(const fcAACEncoderConfig& conf)
{
    mfxStatus ret;
    ret = m_session.Init(MFX_IMPL_AUTO_ANY, nullptr);
    if (ret < 0) {
        return;
    }

    mfxAudioParam params;
    memset(&params, 0, sizeof(params));
    params.mfx.CodecId = MFX_CODEC_AAC;
    params.mfx.CodecProfile = MFX_PROFILE_AAC_MAIN;
    params.mfx.Bitrate = conf.target_bitrate;
    params.mfx.SampleFrequency = conf.sample_rate;
    params.mfx.NumChannel = conf.num_channels;
    params.mfx.BitPerSample = 32;
    params.mfx.OutputFormat = MFX_AUDIO_AAC_ADTS;
    params.mfx.StereoMode = MFX_AUDIO_AAC_LR_STEREO;

    m_encoder.reset(new MFXAudioENCODE(m_session));
    ret = m_encoder->Init(&params);
    if (ret < 0) {
        m_encoder.reset();
    }
}

fcAACEncoderIntel::~fcAACEncoderIntel()
{
    m_encoder.reset();
    m_session.Close();
}

const char* fcAACEncoderIntel::getEncoderInfo()
{
    return "Intel AAC Encoder";
}

const Buffer& fcAACEncoderIntel::getDecoderSpecificInfo()
{
    return m_aac_header;
}

bool fcAACEncoderIntel::encode(fcAACFrame& dst, const float *samples, size_t num_samples, fcTime timestamp)
{
    return false;
}

fcIAACEncoder* fcCreateAACEncoderIntel(const fcAACEncoderConfig& conf)
{
    auto ret = new fcAACEncoderIntel(conf);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

#else // fcSupportAAC_Intel

fcIAACEncoder* fcCreateAACEncoderIntel(const fcAACEncoderConfig& conf) { return nullptr; }

#endif // fcSupportAAC_Intel
