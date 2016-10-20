#pragma once

class fcIWebMWriter
{
public:
    virtual ~fcIWebMWriter() {}
    virtual void setVideoEncoderInfo(const char *id) = 0;
    virtual void setAudioEncoderInfo(const char *id) = 0;

    virtual void addVideoFrame(const fcWebMVideoFrame& buf) = 0;
    virtual void addAudioFrame(const fcWebMAudioFrame& buf) = 0;
};

fcIWebMWriter* fcCreateWebMMuxer(BinaryStream &stream, const fcWebMConfig &conf);
