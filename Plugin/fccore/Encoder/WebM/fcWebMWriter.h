#pragma once

class fcIWebMWriter
{
public:
    virtual ~fcIWebMWriter() {}
    virtual void addVideoFrame(const fcWebMFrameData& buf) = 0;
    virtual void addAudioSamples(const fcWebMFrameData& buf) = 0;
};

fcIWebMWriter* fcCreateWebMWriter(BinaryStream *stream, const fcWebMConfig &conf, const fcIWebMEncoderInfo* vinfo, const fcIWebMEncoderInfo* ainfo);
