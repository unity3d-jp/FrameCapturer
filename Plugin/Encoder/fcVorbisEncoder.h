#pragma once

struct fcVorbisFrame
{
};


struct fcVorbisEncoderConfig
{
    int sampling_rate;
    int num_channels;
    int target_bitrate;
};
typedef fcVorbisEncoderConfig fcOpusEncoderConfig;


class fcIVorbisEncoder
{
public:
    virtual ~fcIVorbisEncoder() {}
    virtual void release() = 0;
    virtual bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) = 0;
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf);
fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf);
