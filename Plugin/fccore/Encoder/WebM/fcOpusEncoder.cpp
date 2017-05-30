#include "pch.h"
#include "fcInternal.h"

#ifdef fcSupportWebM
#include "fcWebMInternal.h"
#include "fcVorbisEncoder.h"

#ifdef fcSupportOpus

#include "opus/opus.h"
#ifdef _MSC_VER
    #pragma comment(lib, "opus.lib")
    #pragma comment(lib, "celt.lib")
    #pragma comment(lib, "silk_common.lib")
    #pragma comment(lib, "silk_float.lib")
#endif // _MSC_VER


class fcOpusEncoder : public fcIWebMAudioEncoder
{
public:
    fcOpusEncoder(const fcOpusEncoderConfig& conf);
    ~fcOpusEncoder() override;
    const char* getMatroskaCodecID() const override;
    const Buffer& getCodecPrivate() const override;

    bool encode(fcWebMFrameData& dst, const float *samples, size_t num_samples) override;
    bool flush(fcWebMFrameData& dst) override;

private:
    fcOpusEncoderConfig m_conf;
    Buffer              m_codec_private;
    RawVector<float>    m_samples;
    Buffer              m_buf_encoded;

    OpusEncoder *m_opus = nullptr;
    int m_preskip = 0;
    uint64_t m_total_samples = 0;
};


fcOpusEncoder::fcOpusEncoder(const fcOpusEncoderConfig& conf)
    : m_conf(conf)
{
    int err;
    m_opus = opus_encoder_create(conf.sample_rate, conf.num_channels, OPUS_APPLICATION_RESTRICTED_LOWDELAY, &err);
    opus_encoder_ctl(m_opus, OPUS_SET_BITRATE(conf.target_bitrate));

    {
        m_codec_private.resize(19);
        auto *cp = (uint8_t*)m_codec_private.data();

        memcpy(cp, "OpusHead", 8);
        cp[8] = 1; // version
        cp[9] = (uint8_t)conf.num_channels;

        // pre-skip
        opus_encoder_ctl(m_opus, OPUS_GET_LOOKAHEAD(&m_preskip));
        (uint16_t&)cp[10] = m_preskip;

        // sample rate
        (uint32_t&)cp[12] = conf.sample_rate;

        // output gain (set to 0)
        cp[16] = cp[17] = 0;

        // channel mapping
        cp[18] = 0;
    }
}

fcOpusEncoder::~fcOpusEncoder()
{
    opus_encoder_destroy(m_opus);
}

const char* fcOpusEncoder::getMatroskaCodecID() const
{
    return "A_OPUS";
}

const Buffer& fcOpusEncoder::getCodecPrivate() const
{
    return m_codec_private;
}

bool fcOpusEncoder::encode(fcWebMFrameData& dst, const float *samples, size_t num_samples)
{
    if (!m_opus || !samples) { return false; }

    m_samples.append(samples, num_samples);

    bool result = true;
    int frame_size = m_conf.sample_rate / 100;
    int processed_size = 0;
    m_buf_encoded.resize(m_conf.sample_rate);

    while (processed_size + frame_size * m_conf.num_channels <= m_samples.size()) {
        opus_int32 n = opus_encode_float(m_opus,
            m_samples.data() + processed_size, frame_size,
            (uint8_t*)m_buf_encoded.data(), (int)m_buf_encoded.size());
        if (n > 0) {
            dst.data.append(m_buf_encoded.data(), n);
            processed_size += frame_size * m_conf.num_channels;
            m_total_samples += frame_size;

            double timestamp = (double)m_total_samples / (double)m_conf.sample_rate;
            dst.packets.push_back({ (uint32_t)n, timestamp, 1 });
        }
        else if (n == 0) {
            break;
        }
        else {
            result = false;
            break;
        }
    }

    m_samples.erase(m_samples.begin(), m_samples.begin() + processed_size);
    return result;
}

bool fcOpusEncoder::flush(fcWebMFrameData& dst)
{
    return true;
}


fcIWebMAudioEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf) { return new fcOpusEncoder(conf); }

#else // fcSupportOpus

fcIWebMAudioEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf) { return nullptr; }

#endif // fcSupportOpus
#endif // fcSupportWebM
