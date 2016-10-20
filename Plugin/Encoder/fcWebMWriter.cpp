#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcVorbisEncoder.h"
#include "fcVPXEncoder.h"
#include "fcWebMWriter.h"

#include "webm/mkvmuxer.hpp"


class fcMkvStream : public mkvmuxer::IMkvWriter
{
public:
    fcMkvStream(BinaryStream &stream) : m_stream(stream) {}
    int32_t Write(const void* buf, uint32_t len) override { m_stream.write(buf, len); return 0; }
    int64_t Position() const override { return m_stream.tellp(); }
    int32_t Position(int64_t position) override { m_stream.seekp(position); return 0; }
    bool Seekable() const override { return true; }
    void ElementStartNotify(uint64_t element_id, int64_t position) override {}
private:
    BinaryStream &m_stream;
};


class fcWebMWriter : public fcIWebMWriter
{
public:
    fcWebMWriter(BinaryStream &stream, const fcWebMConfig &conf);
    ~fcWebMWriter() override;
    void setVideoEncoderInfo(const fcIWebMVideoEncoder& encoder) override;
    void setAudioEncoderInfo(const fcIWebMAudioEncoder& encoder) override;

    void addVideoFrame(const fcWebMVideoFrame& buf) override;
    void addAudioFrame(const fcWebMAudioFrame& buf) override;

private:
    using StreamPtr = std::unique_ptr<fcMkvStream>;

    fcWebMConfig m_conf;
    StreamPtr m_stream;
    std::mutex m_mutex;

    mkvmuxer::Segment m_segment;
    uint64_t m_video_track_id = 0;
    uint64_t m_audio_track_id = 0;
};


fcIWebMWriter* fcCreateWebMMuxer(BinaryStream &stream, const fcWebMConfig &conf)
{
    return new fcWebMWriter(stream, conf);
}

fcWebMWriter::fcWebMWriter(BinaryStream &stream, const fcWebMConfig &conf)
    : m_conf(conf)
    , m_stream(new fcMkvStream(stream))
{
    m_segment.Init(m_stream.get());
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

fcWebMWriter::~fcWebMWriter()
{
    m_segment.Finalize();
}

void fcWebMWriter::setVideoEncoderInfo(const fcIWebMVideoEncoder& encoder)
{
    auto* track = dynamic_cast<mkvmuxer::VideoTrack*>(m_segment.GetTrackByNumber(m_video_track_id));
    if (track) {
        track->set_codec_id(encoder.getMatroskaCodecID());
    }
}

void fcWebMWriter::setAudioEncoderInfo(const fcIWebMAudioEncoder& encoder)
{
    auto* track = dynamic_cast<mkvmuxer::AudioTrack*>(m_segment.GetTrackByNumber(m_audio_track_id));
    if (track) {
        track->set_codec_id(encoder.getMatroskaCodecID());

        const auto& cp = encoder.getCodecPrivate();
        track->SetCodecPrivate((const uint8_t*)cp.data(), cp.size());
    }
}

void fcWebMWriter::addVideoFrame(const fcWebMVideoFrame& frame)
{
    if (m_video_track_id == 0 || frame.data.empty()) { return; }

    std::unique_lock<std::mutex> lock(m_mutex);
    m_segment.AddFrame((const uint8_t*)frame.data.data(), frame.data.size(), m_video_track_id, frame.timestamp, frame.keyframe);
}

void fcWebMWriter::addAudioFrame(const fcWebMAudioFrame& frame)
{
    if (m_audio_track_id == 0 || frame.data.empty()) { return; }

    std::unique_lock<std::mutex> lock(m_mutex);
    m_segment.AddFrame((const uint8_t*)frame.data.data(), frame.data.size(), m_audio_track_id, frame.timestamp, true);
}
