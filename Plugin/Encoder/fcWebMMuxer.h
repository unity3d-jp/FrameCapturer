#pragma once

class fcIWebMMuxer
{
public:
    virtual ~fcIWebMMuxer() {}
    virtual void setVideoEncoderInfo(const char *id) = 0;
    virtual void setAudioEncoderInfo(const char *id) = 0;

    virtual void addVideoFrame(const fcWebMVideoFrame& buf) = 0;
    virtual void addAudioFrame(const fcWebMAudioFrame& buf) = 0;
};

fcIWebMMuxer* fcCreateWebMMuxer(BinaryStream &stream, const fcWebMConfig &conf);
