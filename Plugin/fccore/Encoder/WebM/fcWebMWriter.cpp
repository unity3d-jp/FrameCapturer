#include "pch.h"
#include "fcInternal.h"

#ifdef fcSupportWebM
#include "fcWebMInternal.h"
#include "fcWebMWriter.h"

#include "mkvmuxer.hpp"
#ifdef _MSC_VER
    #pragma comment(lib, "libwebm.lib")
#endif // _MSC_VER


class fcMkvStream : public mkvmuxer::IMkvWriter
{
public:
    fcMkvStream(BinaryStream &stream) : m_stream(stream) {}
    int32_t Write(const void* buf, uint32_t len) override { m_stream.write(buf, len); return 0; }
    mkvmuxer::int64 Position() const override { return m_stream.tellp(); }
    mkvmuxer::int32 Position(mkvmuxer::int64 position) override { m_stream.seekp((size_t)position); return 0; }
    bool Seekable() const override { return true; }
    void ElementStartNotify(mkvmuxer::uint64 element_id, mkvmuxer::int64 position) override {}
private:
    BinaryStream &m_stream;
};


class fcWebMWriter : public fcIWebMWriter
{
public:
    fcWebMWriter(BinaryStream &stream, const fcWebMConfig &conf);
    ~fcWebMWriter() override;
    void setVideoEncoderInfo(const fcIWebMEncoderInfo& info) override;
    void setAudioEncoderInfo(const fcIWebMEncoderInfo& info) override;

    void addVideoFrame(const fcWebMFrameData& buf) override;
    void addAudioFrame(const fcWebMFrameData& buf) override;

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


fcWebMWriter::fcWebMWriter(BinaryStream &stream, const fcWebMConfig &conf)
    : m_conf(conf)
    , m_stream(new fcMkvStream(stream))
{
    m_segment.Init(m_stream.get());
    m_segment.set_mode(mkvmuxer::Segment::kFile);
    m_segment.set_estimate_file_duration(true);
    if (conf.video) {
        m_video_track_id = m_segment.AddVideoTrack(conf.video_width, conf.video_height, 0);
    }
    if(conf.audio) {
        m_audio_track_id = m_segment.AddAudioTrack(conf.audio_sample_rate, conf.audio_num_channels, 0);
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

void fcWebMWriter::setVideoEncoderInfo(const fcIWebMEncoderInfo& info)
{
    auto track = dynamic_cast<mkvmuxer::VideoTrack*>(m_segment.GetTrackByNumber(m_video_track_id));
    if (track) {
        track->set_codec_id(info.getMatroskaCodecID());
    }
}

void fcWebMWriter::setAudioEncoderInfo(const fcIWebMEncoderInfo& info)
{
    auto track = dynamic_cast<mkvmuxer::AudioTrack*>(m_segment.GetTrackByNumber(m_audio_track_id));
    if (track) {
        track->set_codec_id(info.getMatroskaCodecID());

        const auto& cp = info.getCodecPrivate();
        track->SetCodecPrivate((const uint8_t*)cp.data(), cp.size());
    }
}

void fcWebMWriter::addVideoFrame(const fcWebMFrameData& frame)
{
    if (m_video_track_id == 0 || frame.data.empty()) { return; }

    std::unique_lock<std::mutex> lock(m_mutex);
    auto timestamp_prev = m_timestamp_video_last;
    frame.eachPackets([this](const char *data, const fcWebMPacketInfo& pinfo) {
        m_timestamp_video_last = pinfo.timestamp;
        auto mkvf = new mkvmuxer::Frame();
        mkvf->Init((const uint8_t*)data, pinfo.size);
        mkvf->set_track_number(m_video_track_id);
        mkvf->set_timestamp(to_nsec(m_timestamp_video_last));
        mkvf->set_is_key(pinfo.keyframe);
        m_mkvframes.emplace_back(mkvf);
    });
    writeOut(timestamp_prev);
}

void fcWebMWriter::addAudioFrame(const fcWebMFrameData& frame)
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
    std::stable_sort(m_mkvframes.begin(), m_mkvframes.end(),
        [](const MKVFramePtr& a, const MKVFramePtr& b) { return a->timestamp() < b->timestamp(); });

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


fcIWebMWriter* fcCreateWebMWriter(BinaryStream &stream, const fcWebMConfig &conf) { return new fcWebMWriter(stream, conf); }

#else  // fcSupportWebM

fcIWebMWriter* fcCreateWebMWriter(BinaryStream &stream, const fcWebMConfig &conf) { return nullptr; }

#endif // fcSupportWebM
