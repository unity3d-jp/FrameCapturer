#pragma once

#include "mkvmuxer.hpp"

class fcMkvStream;

class fcWebMWriter
{
public:
    static const int VideoTrackIndex = 1;
    static const int AudioTrackIndex = 2;

    fcWebMWriter(BinaryStream *stream, const fcWebMConfig &conf, const fcIWebMVideoEncoder *vinfo, const fcIWebMAudioEncoder *ainfo);
    ~fcWebMWriter();
    void addFrame(mkvmuxer::Frame& f);

private:
    std::unique_ptr<fcMkvStream> m_stream;
    mkvmuxer::Segment m_segment;
    uint64_t m_video_track_id = 0;
    uint64_t m_audio_track_id = 0;
};
