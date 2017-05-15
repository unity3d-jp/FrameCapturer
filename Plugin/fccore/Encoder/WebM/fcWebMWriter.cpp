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
    : m_conf(conf)
    , m_stream(new fcMkvStream(stream))
{
    m_segment.Init(m_stream.get());
    m_segment.set_mode(mkvmuxer::Segment::kFile);
    m_segment.set_estimate_file_duration(true);
    if (conf.video && vinfo) {
        m_video_track_id = m_segment.AddVideoTrack(conf.video_width, conf.video_height, 0);
        auto track = dynamic_cast<mkvmuxer::VideoTrack*>(m_segment.GetTrackByNumber(m_video_track_id));
        track->set_codec_id(vinfo->getMatroskaCodecID());

        mkvmuxer::Colour color;
        color.set_bits_per_channel(10);
        const uint64_t horizontal_subsampling = 1;
        const uint64_t vertical_subsampling = 1;
        color.set_chroma_subsampling_horz(horizontal_subsampling);
        color.set_chroma_subsampling_vert(vertical_subsampling);
        track->SetColour(color);
        track->set_display_width(conf.video_width);
        track->set_display_height(conf.video_height);
        track->set_frame_rate(conf.video_target_framerate);

        m_segment.CuesTrack(m_video_track_id);
    }
    if (conf.audio && ainfo) {
        m_audio_track_id = m_segment.AddAudioTrack(conf.audio_sample_rate, conf.audio_num_channels, 0);
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
    if (m_conf.video) {
        writeOut(m_timestamp_video_last);
    }
    m_segment.Finalize();
}

void fcWebMWriter::addVideoFrame(const fcWebMFrameData& frame)
{
    if (m_video_track_id == 0 || frame.data.empty()) { return; }

    std::unique_lock<std::mutex> lock(m_mutex);
    frame.eachPackets([this](const char *data, const fcWebMPacketInfo& pinfo) {
        m_timestamp_video_last = pinfo.timestamp;
        auto mkvf = new mkvmuxer::Frame();
        mkvf->Init((const uint8_t*)data, pinfo.size);
        mkvf->set_track_number(m_video_track_id);
        mkvf->set_timestamp(to_nsec(m_timestamp_video_last));
        mkvf->set_is_key(pinfo.keyframe);
        m_mkvframes.emplace_back(mkvf);
    });
    writeOut(m_timestamp_video_last - 1.0);
}

void fcWebMWriter::addAudioSamples(const fcWebMFrameData& frame)
{
    if (m_audio_track_id == 0 || frame.data.empty()) { return; }

    std::unique_lock<std::mutex> lock(m_mutex);
    frame.eachPackets([this](const char *data, const fcWebMPacketInfo& pinfo) {
        m_timestamp_audio_last = pinfo.timestamp;
        auto mkvf = new mkvmuxer::Frame();
        mkvf->Init((const uint8_t*)data, pinfo.size);
        mkvf->set_track_number(m_audio_track_id);
        mkvf->set_timestamp(to_nsec(m_timestamp_audio_last));
        mkvf->set_is_key(pinfo.keyframe);
        m_mkvframes.emplace_back(mkvf);
    });
    if (!m_conf.video) {
        writeOut(m_timestamp_audio_last);
    }
}

void fcWebMWriter::writeOut(double timestamp_)
{
    if (timestamp_ < 0.0) { return; }

    if (m_conf.video && m_conf.audio) {
        // must be stable sort
        std::stable_sort(m_mkvframes.begin(), m_mkvframes.end(),
            [](const MKVFramePtr& a, const MKVFramePtr& b) { return a->timestamp() < b->timestamp(); });
    }

    size_t num_added = 0;
    auto timestamp = to_nsec(timestamp_);
    for (auto& mkvf : m_mkvframes) {
        if (mkvf->timestamp() <= timestamp) {
            m_segment.AddGenericFrame(mkvf.get());
            ++num_added;
        }
        else {
            break;
        }
    }
    m_mkvframes.erase(m_mkvframes.begin(), m_mkvframes.begin() + num_added);
}

#endif // fcSupportWebM
