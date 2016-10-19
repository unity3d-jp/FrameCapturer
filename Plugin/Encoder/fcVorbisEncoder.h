#pragma once

struct fcVorbisFrame
{
};


struct fcVorbisEncoderConfig
{
};

struct fcOpusEncoderConfig
{
};


class fcIVorbisEncoder
{
public:
    virtual void release() = 0;
    virtual bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) = 0;
protected:
    virtual ~fcIVorbisEncoder() {}
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf);
fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf);
