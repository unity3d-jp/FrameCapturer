#pragma once

struct fcVPXEncoderConfig
{
    int width = 1920;
    int height = 1080;
    int target_bitrate = 0;
    int max_framerate = 60;
};

struct fcVP8EncoderConfig : public fcVPXEncoderConfig
{
};

struct fcVP9EncoderConfig : public fcVPXEncoderConfig
{
};

struct fcVPXFrame
{
};


class fcIVPXEncoder
{
public:
    virtual void release() = 0;
    virtual bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe = false) = 0;

protected:
    virtual ~fcIVPXEncoder() {}
};

fcIVPXEncoder* fcCreateVP8Encoder(const fcVP8EncoderConfig& conf);
fcIVPXEncoder* fcCreateVP9Encoder(const fcVP9EncoderConfig& conf);
