#pragma once

struct fcVPXEncoderConfig
{
    int width = 0;
    int height = 0;
    int target_bitrate = 0;
    int max_framerate = 60;
};
typedef fcVPXEncoderConfig fcVP8EncoderConfig;
typedef fcVPXEncoderConfig fcVP9EncoderConfig;

struct fcVPXFrame
{
    fcTime timestamp;
    Buffer data;
    std::vector<int> segments;
};


class fcIVPXEncoder
{
public:
    virtual ~fcIVPXEncoder() {}
    virtual void release() = 0;
    virtual bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe = false) = 0;
};

fcIVPXEncoder* fcCreateVP8Encoder(const fcVP8EncoderConfig& conf);
fcIVPXEncoder* fcCreateVP9Encoder(const fcVP9EncoderConfig& conf);
