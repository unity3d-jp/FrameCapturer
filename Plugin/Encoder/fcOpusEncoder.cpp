#include "pch.h"
#include "fcFoundation.h"
#include "fcVorbisEncoder.h"

#include "opus/opus.h"


class fcOpusEncoder : public fcIVorbisEncoder
{
public:
    fcOpusEncoder(const fcOpusEncoderConfig& conf);
    ~fcOpusEncoder() override;

    void release() override;
    bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) override;

private:
    fcOpusEncoderConfig m_conf;
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcOpusEncoderConfig& conf)
{
    return new fcOpusEncoder(conf);
}


fcOpusEncoder::fcOpusEncoder(const fcOpusEncoderConfig& conf)
    : m_conf(conf)
{
}

fcOpusEncoder::~fcOpusEncoder()
{
}

void fcOpusEncoder::release()
{
    delete this;
}

bool fcOpusEncoder::encode(fcVorbisFrame& dst, const float *samples, size_t num_samples)
{
    return false;
}

