#pragma once

struct fcVPXEncoderConfig
{
    int width;
    int height;
    int target_framerate;
    fcBitrateMode bitrate_mode;
    int target_bitrate;
};


fcIWebMVideoEncoder* fcCreateVP8EncoderVPX(const fcVPXEncoderConfig& conf);
fcIWebMVideoEncoder* fcCreateVP9EncoderVPX(const fcVPXEncoderConfig& conf);
fcIWebMVideoEncoder* fcCreateVP9LossLessEncoderVPX(const fcVPXEncoderConfig& conf);
