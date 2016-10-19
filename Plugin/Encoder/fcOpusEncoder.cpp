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
    const char* getMatroskaCodecID() override;

    bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) override;

private:
    fcOpusEncoderConfig m_conf;
};


fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf)
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

const char* fcOpusEncoder::getMatroskaCodecID()
{
    return "A_OPUS";
}

bool fcOpusEncoder::encode(fcVorbisFrame& dst, const float *samples, size_t num_samples)
{
    return false;
}

