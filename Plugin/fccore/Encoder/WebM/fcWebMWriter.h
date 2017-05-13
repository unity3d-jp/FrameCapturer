#pragma once

class fcIWebMWriter
{
public:
    virtual ~fcIWebMWriter() {}
    virtual void setVideoEncoderInfo(const fcIWebMEncoderInfo& info) = 0;
    virtual void setAudioEncoderInfo(const fcIWebMEncoderInfo& info) = 0;

    virtual void addVideoFrame(const fcWebMFrameData& buf) = 0;
    virtual void AddAudioSamples(const fcWebMFrameData& buf) = 0;
};

fcIWebMWriter* fcCreateWebMWriter(BinaryStream *stream, const fcWebMConfig &conf);
