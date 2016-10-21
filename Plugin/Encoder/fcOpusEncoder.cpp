#include "pch.h"
#include "fcFoundation.h"
#include "fcVorbisEncoder.h"

#ifdef fcSupportOpus

#include "opus/opus.h"
#ifdef _MSC_VER
    #pragma comment(lib, "opus.lib")
    #pragma comment(lib, "celt.lib")
    #pragma comment(lib, "silk_common.lib")
    #pragma comment(lib, "silk_float.lib")
#endif // _MSC_VER


class fcOpusEncoder : public fcIVorbisEncoder
{
public:
    fcOpusEncoder(const fcOpusEncoderConfig& conf);
    ~fcOpusEncoder() override;
    void release() override;
    const char* getMatroskaCodecID() const override;
    const Buffer& getCodecPrivate() const override;

    bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) override;
    bool flush(fcVorbisFrame& dst) override;

private:
    fcOpusEncoderConfig m_conf;
    Buffer m_codec_private;
    RawVector<float> m_samples;
    Buffer m_buf_encoded;

    OpusEncoder *m_op_encoder = nullptr;
};


fcOpusEncoder::fcOpusEncoder(const fcOpusEncoderConfig& conf)
    : m_conf(conf)
{
    int err;
    m_op_encoder = opus_encoder_create(conf.sample_rate, conf.num_channels, OPUS_APPLICATION_RESTRICTED_LOWDELAY, &err);
    opus_encoder_ctl(m_op_encoder, OPUS_SET_BITRATE(conf.target_bitrate));
}

fcOpusEncoder::~fcOpusEncoder()
{
    opus_encoder_destroy(m_op_encoder);
}

void fcOpusEncoder::release()
{
    delete this;
}

const char* fcOpusEncoder::getMatroskaCodecID() const
{
    return "A_OPUS";
}

const Buffer& fcOpusEncoder::getCodecPrivate() const
{
    return m_codec_private;
}

bool fcOpusEncoder::encode(fcVorbisFrame& dst, const float *samples, size_t num_samples)
{
    if (!m_op_encoder || !samples) { return false; }

    return false;
    // todo: implement

    //m_samples.append(samples, num_samples);

    //int block_size = m_conf.sample_rate * m_conf.num_channels / 10;
    //int processed_size = 0;
    //m_buf_encoded.resize(block_size);
    //while (processed_size + block_size <= num_samples) {
    //    auto n = opus_encode_float(m_op_encoder,
    //        m_samples.data() + processed_size, block_size / m_conf.num_channels,
    //        (uint8_t*)m_buf_encoded.data(), (int)m_buf_encoded.size());
    //    if (n > 0) {
    //        m_buf_encoded.resize(n);
    //        dst.data.append(m_buf_encoded.data(), m_buf_encoded.size());
    //    }
    //    else if (n == 0) {
    //        break;
    //    }
    //    else {
    //        return false;
    //    }
    //}
    //return true;
}

bool fcOpusEncoder::flush(fcVorbisFrame& dst)
{
    return true;
}


fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf) { return new fcOpusEncoder(conf); }

#else // fcSupportOpus

fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf) { return nullptr; }

#endif // fcSupportOpus
