#pragma once


struct fcVorbisEncoderConfig
{
    int sample_rate;
    int num_channels;
    int target_bitrate;
};
using fcOpusEncoderConfig = fcVorbisEncoderConfig;

struct fcVorbisFrame
{
    Buffer data;
    std::vector<int> segments;
    uint64_t timestamp = 0;

    void clear()
    {
        data.clear();
        segments.clear();
        timestamp = 0;
    }
};

class fcIVorbisEncoder
{
public:
    virtual ~fcIVorbisEncoder() {}
    virtual void release() = 0;
    virtual const char* getMatroskaCodecID() const = 0;
    virtual const Buffer& getCodecPrivate() const = 0;

    virtual bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) = 0;
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf);
fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf);

using fcWebMAudioEncoderConfig = fcVorbisEncoderConfig;
using fcWebMAudioFrame = fcVorbisFrame;
using fcIWebMAudioEncoder = fcIVorbisEncoder;
