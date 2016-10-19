#pragma once

class fcWebMMuxer
{
public:
    fcWebMMuxer(BinaryStream &stream, const fcWebMConfig &conf);
    virtual ~fcWebMMuxer();
    void addVideoFrame(const fcVPXFrame& buf);
    void addAudioFrame(const fcVorbisFrame& buf);

private:
    BinaryStream& m_stream;
    fcWebMConfig m_conf;
    std::mutex m_mutex;
};
