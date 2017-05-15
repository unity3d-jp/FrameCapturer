#pragma once

struct fcVPXEncoderConfig
{
    int width;
    int height;
    int target_framerate;
    fcBitrateMode bitrate_mode;
    int target_bitrate;
};


fcIWebMVideoEncoder* fcCreateVPXVP8Encoder(const fcVPXEncoderConfig& conf);
fcIWebMVideoEncoder* fcCreateVPXVP9Encoder(const fcVPXEncoderConfig& conf);
fcIWebMVideoEncoder* fcCreateVPXVP9LossLessEncoder(const fcVPXEncoderConfig& conf);
