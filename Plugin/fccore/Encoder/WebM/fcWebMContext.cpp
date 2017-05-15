#include "pch.h"
#include "fcInternal.h"

#ifdef fcSupportWebM
#include "fcWebMInternal.h"
#include "fcWebMContext.h"
#include "fcWebMWriter.h"
#include "fcVorbisEncoder.h"
#include "fcVPXEncoder.h"


class fcWebMContext : public fcIWebMContext
{
public:
    using VideoEncoderPtr   = std::unique_ptr<fcIWebMVideoEncoder>;
    using AudioEncoderPtr   = std::unique_ptr<fcIWebMAudioEncoder>;
    using WriterPtr         = std::unique_ptr<fcWebMWriter>;
    using WriterPtrs        = std::vector<WriterPtr>;
    using VideoBuffer       = Buffer;
    using VideoBuffers      = SharedResources<VideoBuffer>;
    using AudioBuffer       = RawVector<float>;
    using AudioBuffers      = SharedResources<AudioBuffer>;
    using MKVFramePtr       = std::unique_ptr<mkvmuxer::Frame>;
    using MKVFramePtrs      = std::vector<MKVFramePtr>;


    fcWebMContext(fcWebMConfig &conf, fcIGraphicsDevice *gd);
    void addOutputStream(fcStream *s) override;
    bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp) override;
    bool addAudioSamples(const float *samples, int num_samples) override;

private:
    ~fcWebMContext() override;
    void addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamp);
    void flushVideo();
    void flushAudio();
    void addMkvFrames(fcWebMFrameData& data, int track, double& last_timestamp);
    void writeOut(double timestamp);

private:
    fcWebMConfig        m_conf;
    fcIGraphicsDevice   *m_gdev = nullptr;

    std::mutex          m_mutex;
    WriterPtrs          m_writers;
    MKVFramePtrs        m_mkv_frames;

    TaskQueue           m_video_tasks;
    VideoEncoderPtr     m_video_encoder;
    VideoBuffers        m_video_buffers;
    fcWebMFrameData     m_video_frame;
    double              m_video_last_timestamp = 0.0;

    TaskQueue           m_audio_tasks;
    AudioEncoderPtr     m_audio_encoder;
    AudioBuffers        m_audio_buffers;
    fcWebMFrameData     m_audio_frame;
    double              m_audio_last_timestamp = 0.0;
};


fcWebMContext::fcWebMContext(fcWebMConfig &conf, fcIGraphicsDevice *gd)
    : m_conf(conf)
    , m_gdev(gd)
{
    m_conf.video_max_tasks = std::max<int>(m_conf.video_max_tasks, 1);
    m_conf.audio_max_tasks = std::max<int>(m_conf.audio_max_tasks, 1);

    if (conf.video) {
        fcVPXEncoderConfig econf;
        econf.width = conf.video_width;
        econf.height = conf.video_height;
        econf.target_framerate = conf.video_target_framerate;
        econf.bitrate_mode = conf.video_bitrate_mode;
        econf.target_bitrate = conf.video_target_bitrate;

        switch (conf.video_encoder) {
        case fcWebMVideoEncoder::VPX_VP8:
            m_video_encoder.reset(fcCreateVPXVP8Encoder(econf));
            break;
        case fcWebMVideoEncoder::VPX_VP9:
            m_video_encoder.reset(fcCreateVPXVP9Encoder(econf));
            break;
        case fcWebMVideoEncoder::VPX_VP9LossLess:
            m_video_encoder.reset(fcCreateVPXVP9LossLessEncoder(econf));
            break;
        }

        for (int i = 0; i < m_conf.video_max_tasks; ++i) {
            m_video_buffers.emplace();
        }
    }

    if (conf.audio) {
        fcVorbisEncoderConfig econf;
        econf.sample_rate = conf.audio_sample_rate;
        econf.num_channels = conf.audio_num_channels;
        econf.bitrate_mode = conf.audio_bitrate_mode;
        econf.target_bitrate = conf.audio_target_bitrate;

        switch (conf.audio_encoder) {
        case fcWebMAudioEncoder::Vorbis:
            m_audio_encoder.reset(fcCreateVorbisEncoder(econf));
            break;
        case fcWebMAudioEncoder::Opus:
            m_audio_encoder.reset(fcCreateOpusEncoder(econf));
            break;
        }

        for (int i = 0; i < m_conf.audio_max_tasks; ++i) {
            m_audio_buffers.emplace();
        }
    }
}

fcWebMContext::~fcWebMContext()
{
    flushVideo();
    flushAudio();
    m_video_tasks.wait();
    m_audio_tasks.wait();

    if (m_conf.video) {
        writeOut(m_video_last_timestamp);
    }
    m_mkv_frames.clear();
    m_writers.clear();

    m_video_encoder.reset();
    m_audio_encoder.reset();
}

void fcWebMContext::addOutputStream(fcStream *s)
{
    if (!s) { return; }
    m_writers.emplace_back(new fcWebMWriter(s, m_conf, m_video_encoder.get(), m_audio_encoder.get()));
}

bool fcWebMContext::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!tex || !m_video_encoder || !m_gdev) { return false; }

    auto buf = m_video_buffers.acquire();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    if (m_gdev->readTexture(buf->data(), buf->size(), tex, m_conf.video_width, m_conf.video_height, fmt)) {
        m_video_tasks.run([this, buf, fmt, timestamp]() {
            addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
        });
    }
    else {
        return false;
    }
    return true;
}

bool fcWebMContext::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!pixels || !m_video_encoder) { return false; }

    auto buf = m_video_buffers.acquire();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    memcpy(buf->data(), pixels, size);

    m_video_tasks.run([this, buf, fmt, timestamp]() {
        addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
    });
    return true;
}

void fcWebMContext::addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (m_video_encoder->encode(m_video_frame, pixels, fmt, timestamp)) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            addMkvFrames(m_video_frame, fcWebMWriter::VideoTrackIndex, m_video_last_timestamp);
            writeOut(m_video_last_timestamp - 1.0);
        }
        m_video_frame.clear();
    }
}

void fcWebMContext::flushVideo()
{
    if (!m_video_encoder) { return; }

    m_video_tasks.run([this]() {
        if (m_video_encoder->flush(m_video_frame)) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                addMkvFrames(m_video_frame, fcWebMWriter::VideoTrackIndex, m_video_last_timestamp);
            }
            m_video_frame.clear();
        }
    });
}


bool fcWebMContext::addAudioSamples(const float *samples, int num_samples)
{
    if (!samples || !m_audio_encoder) { return false; }

    auto buf = m_audio_buffers.acquire();
    buf->assign(samples, num_samples);

    m_audio_tasks.run([this, buf]() {
        if (m_audio_encoder->encode(m_audio_frame, buf->data(), buf->size())) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                addMkvFrames(m_audio_frame, fcWebMWriter::AudioTrackIndex, m_audio_last_timestamp);
                if (!m_conf.video) {
                    writeOut(m_audio_last_timestamp);
                }
            }
            m_audio_frame.clear();
        }
    });
    return true;
}

void fcWebMContext::flushAudio()
{
    if (!m_audio_encoder) { return; }

    m_audio_tasks.run([this]() {
        if (m_audio_encoder->flush(m_audio_frame)) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                addMkvFrames(m_audio_frame, fcWebMWriter::AudioTrackIndex, m_audio_last_timestamp);
                if (!m_conf.video) {
                    writeOut(m_audio_last_timestamp);
                }
            }
            m_audio_frame.clear();
        }
    });
}

void fcWebMContext::addMkvFrames(fcWebMFrameData& fd, int track, double& last_timestamp)
{
    fd.eachPackets([this, track, &last_timestamp](const char *data, const fcWebMPacketInfo& pinfo) {
        last_timestamp = pinfo.timestamp;
        auto mkvf = new mkvmuxer::Frame();
        mkvf->Init((const uint8_t*)data, pinfo.size);
        mkvf->set_track_number(track);
        mkvf->set_timestamp(to_nsec(last_timestamp));
        mkvf->set_is_key(pinfo.keyframe);
        m_mkv_frames.emplace_back(mkvf);
    });
}

void fcWebMContext::writeOut(double timestamp_)
{
    if (timestamp_ < 0.0) { return; }

    if (m_conf.video && m_conf.audio) {
        std::stable_sort(m_mkv_frames.begin(), m_mkv_frames.end(),
            [](const MKVFramePtr& a, const MKVFramePtr& b) { return a->timestamp() < b->timestamp(); });
    }

    int num_added = 0;
    auto timestamp = to_nsec(timestamp_);
    for (auto& mkvf : m_mkv_frames) {
        if (mkvf->timestamp() <= timestamp) {
            for (auto& w : m_writers) {
                w->addFrame(*mkvf);
            }
            ++num_added;
        }
        else {
            break;
        }
    }
    m_mkv_frames.erase(m_mkv_frames.begin(), m_mkv_frames.begin() + num_added);
}



fcIWebMContext* fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice *gd) { return new fcWebMContext(conf, gd); }

#else  // fcSupportWebM

fcIWebMContext* fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice *gd) { return nullptr; }

#endif // fcSupportWebM
