#ifndef fcMP4StreamWriter_h
#define fcMP4StreamWriter_h

class fcMP4StreamWriter
{
public:
    fcMP4StreamWriter(BinaryStream &stream, const fcMP4Config &conf);
    virtual ~fcMP4StreamWriter();
    void addFrame(const fcFrameData& buf);

private:
    void mp4Begin();
    void mp4End();

private:
    BinaryStream& m_stream;
    fcMP4Config m_conf;
    std::vector<fcFrameInfo> m_video_frame_info;
    std::vector<fcFrameInfo> m_audio_frame_info;
    fcFrameData m_video_header;
    fcFrameData m_audio_header;
    std::vector<u8> m_pps;
    std::vector<u8> m_sps;

    std::vector<fcSampleToChunk> m_video_samples_to_chunk;
    std::vector<fcSampleToChunk> m_audio_samples_to_chunk;
    std::vector<fcOffsetValue> m_video_decode_times;
    std::vector<fcOffsetValue> m_audio_decode_times;
    std::vector<u64> m_video_chunks;
    std::vector<u64> m_audio_chunks;
    std::vector<u32> m_iframe_ids;

    size_t m_mdat_begin;
};

#endif // fcMP4StreamWriter_h
