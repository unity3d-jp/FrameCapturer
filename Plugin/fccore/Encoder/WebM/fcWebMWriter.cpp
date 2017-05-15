#include "pch.h"
#include "fcInternal.h"

#ifdef fcSupportWebM
#include "fcWebMInternal.h"
#include "fcWebMWriter.h"

#ifdef _MSC_VER
    #pragma comment(lib, "libwebm.lib")
#endif // _MSC_VER


class fcMkvStream : public mkvmuxer::IMkvWriter
{
public:
    fcMkvStream(BinaryStream *stream) : m_stream(stream) { m_stream->addRef(); }
    ~fcMkvStream() { m_stream->release(); }
    int32_t Write(const void* buf, uint32_t len) override { m_stream->write(buf, len); return 0; }
    mkvmuxer::int64 Position() const override { return m_stream->tellp(); }
    mkvmuxer::int32 Position(mkvmuxer::int64 position) override { m_stream->seekp((size_t)position); return 0; }
    bool Seekable() const override { return true; }
    void ElementStartNotify(mkvmuxer::uint64 element_id, mkvmuxer::int64 position) override {}
private:
    BinaryStream *m_stream = nullptr;
};


fcWebMWriter::fcWebMWriter(
    BinaryStream *stream, const fcWebMConfig &conf,
    const fcIWebMVideoEncoder *vinfo, const fcIWebMAudioEncoder *ainfo)
    : m_stream(new fcMkvStream(stream))
{
    m_segment.Init(m_stream.get());
    m_segment.set_mode(mkvmuxer::Segment::kFile);
    m_segment.set_estimate_file_duration(true);

    if (conf.video && vinfo) {
        m_video_track_id = m_segment.AddVideoTrack(conf.video_width, conf.video_height, VideoTrackIndex);
        auto track = dynamic_cast<mkvmuxer::VideoTrack*>(m_segment.GetTrackByNumber(m_video_track_id));
        track->set_codec_id(vinfo->getMatroskaCodecID());
        track->set_display_width(conf.video_width);
        track->set_display_height(conf.video_height);
        track->set_frame_rate(conf.video_target_framerate);

        m_segment.CuesTrack(m_video_track_id);
    }

    if (conf.audio && ainfo) {
        m_audio_track_id = m_segment.AddAudioTrack(conf.audio_sample_rate, conf.audio_num_channels, AudioTrackIndex);
        auto track = dynamic_cast<mkvmuxer::AudioTrack*>(m_segment.GetTrackByNumber(m_audio_track_id));
        track->set_codec_id(ainfo->getMatroskaCodecID());

        const auto& cp = ainfo->getCodecPrivate();
        if (!cp.empty()) {
            track->SetCodecPrivate((const uint8_t*)cp.data(), cp.size());
        }

        if (!conf.video) {
            m_segment.CuesTrack(m_audio_track_id);
        }
    }

    auto info = m_segment.GetSegmentInfo();
    info->set_writing_app("Unity Movie Recorder");
}

fcWebMWriter::~fcWebMWriter()
{
    m_segment.Finalize();
}

void fcWebMWriter::addFrame(mkvmuxer::Frame& f)
{
    m_segment.AddGenericFrame(&f);
}


#endif // fcSupportWebM
