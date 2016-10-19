#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcVorbisEncoder.h"
#include "fcVPXEncoder.h"
#include "fcWebMMuxer.h"

#include "webm/mkvmuxer.hpp"


class fcMkvWriter : public mkvmuxer::IMkvWriter
{
public:
    fcMkvWriter(BinaryStream &stream) : m_stream(stream) {}
    int32_t Write(const void* buf, uint32_t len) override { m_stream.write(buf, len); return 0; }
    int64_t Position() const override { return m_stream.tellp(); }
    int32_t Position(int64_t position) override { m_stream.seekp(position); return 0; }
    bool Seekable() const override { return true; }
    void ElementStartNotify(uint64_t element_id, int64_t position) override {}
private:
    BinaryStream &m_stream;
};


class fcWebMMuxer : public fcIWebMMuxer
{
public:
    fcWebMMuxer(BinaryStream &stream, const fcWebMConfig &conf);
    ~fcWebMMuxer() override;
    void setVideoEncoderInfo(const char *id) override;
    void setAudioEncoderInfo(const char *id) override;

    void addVideoFrame(const fcWebMVideoFrame& buf) override;
    void addAudioFrame(const fcWebMAudioFrame& buf) override;

private:
    std::unique_ptr<fcMkvWriter> m_writer;
    fcWebMConfig m_conf;
    std::mutex m_mutex;

    mkvmuxer::Segment m_segment;
    uint64_t m_video_track_id = 0;
    uint64_t m_audio_track_id = 0;
};


fcIWebMMuxer* fcCreateWebMMuxer(BinaryStream &stream, const fcWebMConfig &conf)
{
    return new fcWebMMuxer(stream, conf);
}

fcWebMMuxer::fcWebMMuxer(BinaryStream &stream, const fcWebMConfig &conf)
    : m_writer(new fcMkvWriter(stream))
    , m_conf(conf)
{
    m_segment.Init(m_writer.get());
    m_segment.set_mode(mkvmuxer::Segment::kLive);
    if (conf.video) {
        m_video_track_id = m_segment.AddVideoTrack(conf.video_width, conf.video_height, 0);
    }
    if(conf.audio) {
        m_audio_track_id = m_segment.AddAudioTrack(conf.audio_sample_rate, conf.audio_num_channels, 0);
    }

    mkvmuxer::SegmentInfo* const info = m_segment.GetSegmentInfo();
    info->set_writing_app("Unity WebM Recorder");
}

fcWebMMuxer::~fcWebMMuxer()
{
    m_segment.Finalize();
}

void fcWebMMuxer::setVideoEncoderInfo(const char *id)
{
    auto* track = dynamic_cast<mkvmuxer::VideoTrack*>(m_segment.GetTrackByNumber(m_video_track_id));
    track->set_codec_id(id);
}

void fcWebMMuxer::setAudioEncoderInfo(const char *id)
{
    auto* track = dynamic_cast<mkvmuxer::AudioTrack*>(m_segment.GetTrackByNumber(m_audio_track_id));
    track->set_codec_id(id);
}

void fcWebMMuxer::addVideoFrame(const fcWebMVideoFrame& frame)
{
    m_segment.AddFrame((uint8_t*)frame.data.data(), frame.data.size(), m_video_track_id, frame.timestamp, frame.keyframe);
}

void fcWebMMuxer::addAudioFrame(const fcWebMAudioFrame& frame)
{
    m_segment.AddFrame((uint8_t*)frame.data.data(), frame.data.size(), m_audio_track_id, frame.timestamp, false);
}
