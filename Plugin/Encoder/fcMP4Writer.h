#pragma once

class fcMP4Writer
{
public:
    fcMP4Writer(BinaryStream &stream, const fcMP4Config &conf);
    virtual ~fcMP4Writer();
    void addFrame(const fcFrameData& buf); // thread safe
    void setAACEncoderInfo(const Buffer& aacheader);

private:
    void mp4Begin();
    void mp4End();

private:
    BinaryStream& m_stream;
    fcMP4Config m_conf;
    std::mutex m_mutex;
    std::vector<fcFrameInfo> m_video_frame_info;
    std::vector<fcFrameInfo> m_audio_frame_info;
    std::vector<u8> m_pps;
    std::vector<u8> m_sps;
    std::vector<u32> m_iframe_ids;
    std::vector<u8> m_audio_encoder_info;

    size_t m_mdat_begin;
    size_t m_mdat_end;
};
