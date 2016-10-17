#pragma once

struct fcVPXEncoderConfig
{
    int width = 1920;
    int height = 1080;
    int target_bitrate = 0;
    int max_framerate = 60;
};

struct fcVPXFrame
{
};


class fcIVPXEncoder
{
public:
    virtual void release() = 0;
    virtual const char* getEncoderInfo() = 0;
    virtual bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe = false) = 0;

protected:
    virtual ~fcIVPXEncoder() = 0;
};

fcIVPXEncoder* fcCreateVP8Encoder(const fcVPXEncoderConfig& conf);
fcIVPXEncoder* fcCreateVP9Encoder(const fcVPXEncoderConfig& conf);
