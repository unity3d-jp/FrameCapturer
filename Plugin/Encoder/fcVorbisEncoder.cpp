#include "pch.h"
#include "fcFoundation.h"
#include "fcVorbisEncoder.h"

#include "vorbis/vorbisenc.h"


class fcVorbisEncoder : public fcIVorbisEncoder
{
public:
    fcVorbisEncoder(const fcVorbisEncoderConfig& conf);
    ~fcVorbisEncoder() override;

    void release() override;
    bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) override;

private:
    fcVorbisEncoderConfig m_conf;
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf)
{
    return new fcVorbisEncoder(conf);
}


fcVorbisEncoder::fcVorbisEncoder(const fcVorbisEncoderConfig& conf)
    : m_conf(conf)
{
}

fcVorbisEncoder::~fcVorbisEncoder()
{
}

void fcVorbisEncoder::release()
{
    delete this;
}

bool fcVorbisEncoder::encode(fcVorbisFrame& dst, const float *samples, size_t num_samples)
{
    return false;
}

