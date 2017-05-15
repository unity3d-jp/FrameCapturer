#pragma once

#include "mkvmuxer.hpp"

class fcMkvStream;

class fcWebMWriter
{
public:
    fcWebMWriter(BinaryStream *stream, const fcWebMConfig &conf, const fcIWebMVideoEncoder *vinfo, const fcIWebMAudioEncoder *ainfo);
    ~fcWebMWriter();
    void addVideoFrame(const fcWebMFrameData& buf);
    void addAudioSamples(const fcWebMFrameData& buf);

    void writeOut(double timestamp);

private:
    using StreamPtr = std::unique_ptr<fcMkvStream>;
    using MKVFramePtr = std::unique_ptr<mkvmuxer::Frame>;

    fcWebMConfig m_conf;
    StreamPtr m_stream;
    std::mutex m_mutex;

    mkvmuxer::Segment m_segment;
    uint64_t m_video_track_id = 0;
    uint64_t m_audio_track_id = 0;

    std::vector<MKVFramePtr> m_mkvframes;
    double m_timestamp_video_last = 0.0;
    double m_timestamp_audio_last = 0.0;
};
