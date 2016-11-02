#pragma once



class fcMP4Writer
{
public:
    fcMP4Writer(BinaryStream &stream, const fcMP4Config &conf);
    virtual ~fcMP4Writer();
    void addVideoFrame(const fcH264Frame& frame); // thread safe
    void addAudioFrame(const fcAACFrame& frame); // thread safe
    void setAACEncoderInfo(const Buffer& aacheader);

private:
    void mp4Begin();
    void mp4End();

private:
    BinaryStream& m_stream;
    fcMP4Config m_conf;
    std::mutex m_mutex;
    RawVector<fcMP4FrameInfo> m_video_frame_info;
    RawVector<fcMP4FrameInfo> m_audio_frame_info;
    RawVector<u8> m_pps;
    RawVector<u8> m_sps;
    RawVector<u32> m_iframe_ids;
    RawVector<u8> m_audio_encoder_info;

    size_t m_mdat_begin;
    size_t m_mdat_end;
};
