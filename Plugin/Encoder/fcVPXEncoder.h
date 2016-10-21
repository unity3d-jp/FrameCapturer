#pragma once

struct fcVPXEncoderConfig
{
    int width = 0;
    int height = 0;
    int target_bitrate = 0;
    int max_framerate = 60;
};

struct fcVPXFrame
{
    Buffer data;
    RawVector<int> segments;
    uint64_t timestamp = 0;
    uint32_t keyframe : 1;

    void clear()
    {
        data.clear();
        segments.clear();
        timestamp = 0;
        keyframe = 0;
    }
};


class fcIVPXEncoder
{
public:
    virtual ~fcIVPXEncoder() {}
    virtual void release() = 0;
    virtual const char* getMatroskaCodecID() const = 0;

    virtual bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe = false) = 0;
    virtual bool flush(fcVPXFrame& dst) = 0;
};

fcIVPXEncoder* fcCreateVP8Encoder(const fcVPXEncoderConfig& conf);
fcIVPXEncoder* fcCreateVP9Encoder(const fcVPXEncoderConfig& conf);

using fcWebMVideoEncoderConfig = fcVPXEncoderConfig;
using fcWebMVideoFrame = fcVPXFrame;
using fcIWebMVideoEncoder = fcIVPXEncoder;
