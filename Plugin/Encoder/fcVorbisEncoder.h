#pragma once

struct fcVorbisFrame
{
};


struct fcVorbisEncoderConfig
{
};

class fcVorbisEncoder
{
public:
    virtual void release() = 0;
    virtual bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) = 0;

protected:
    virtual ~fcVorbisEncoder() = 0;
};


fcVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf);
