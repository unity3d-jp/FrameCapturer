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

    template<class F>
    void eachFrame(const F &body)
    {
        for (auto& v : m_frame_info) {
            body(v);
        }
    }

    template<class F>
    void eachVideoFrame(const F &body)
    {
        for (auto& v : m_frame_info) {
            if (v.type == fcFrameType_H264) body(v);
        }
    }

    template<class F>
    void eachAudioFrame(const F &body)
    {
        for (auto& v : m_frame_info) {
            if (v.type == fcFrameType_AAC) body(v);
        }
    }

private:
    BinaryStream& m_stream;
    fcMP4Config m_conf;
    std::vector<fcFrameInfo> m_frame_info;
    fcFrameData m_video_header;
    fcFrameData m_audio_header;
    std::vector<u8> m_pps;
    std::vector<u8> m_sps;

    std::vector<fcSampleToChunk> m_video_samples_to_chunk;
    std::vector<fcSampleToChunk> m_audio_sample_to_chunk;
    std::vector<fcOffsetValue> m_video_decode_times;
    std::vector<fcOffsetValue> m_audio_decode_times;
    std::vector<fcOffsetValue> m_composition_offsets;
    std::vector<u64> m_video_chunks;
    std::vector<u64> m_audio_chunks;
    std::vector<u32> m_iframe_ids;
    size_t m_mdat_begin;
};

#endif // fcMP4StreamWriter_h
