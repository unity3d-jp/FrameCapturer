#pragma once


struct fcVorbisEncoderConfig
{
    int sample_rate;
    int num_channels;
    fcBitrateMode bitrate_mode;
    int target_bitrate;
};
using fcOpusEncoderConfig = fcVorbisEncoderConfig;

fcIWebMAudioEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf);
fcIWebMAudioEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf);

using fcWebMAudioEncoderConfig = fcVorbisEncoderConfig;
using fcWebMFrameData = fcWebMFrameData;
using fcIWebMAudioEncoder = fcIWebMAudioEncoder;
