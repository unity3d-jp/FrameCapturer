#include "pch.h"
#include "fcFoundation.h"
#include "fcVorbisEncoder.h"

#include "vorbis/vorbisenc.h"


class fcVorbisEncoder : public fcIVorbisEncoder
{
public:
    fcVorbisEncoder(const fcVorbisEncoderConfig& conf);
    ~fcVorbisEncoder() override;
    void release() override;
    const char* getMatroskaCodecID() override;

    bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) override;

private:
    void gatherPacketData(fcVorbisFrame& dst);

    fcVorbisEncoderConfig m_conf;
    vorbis_info      m_vo_info;
    vorbis_comment   m_vo_comment;
    vorbis_dsp_state m_vo_dsp;
    vorbis_block     m_vo_block;
    ogg_stream_state m_og_ss;
    int m_frame = 0;
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf)
{
    return new fcVorbisEncoder(conf);
}


fcVorbisEncoder::fcVorbisEncoder(const fcVorbisEncoderConfig& conf)
    : m_conf(conf)
{
    vorbis_info_init(&m_vo_info);
    vorbis_encode_init(&m_vo_info, conf.num_channels, conf.sample_rate, -1, conf.target_bitrate, -1);

    vorbis_comment_init(&m_vo_comment);
    vorbis_comment_add_tag(&m_vo_comment, "ENCODER", "Unity WebM Recorder");

    vorbis_analysis_init(&m_vo_dsp, &m_vo_info);
    vorbis_block_init(&m_vo_dsp, &m_vo_block);

    ogg_stream_init(&m_og_ss, 0);
}

fcVorbisEncoder::~fcVorbisEncoder()
{
    ogg_stream_clear(&m_og_ss);
    vorbis_block_clear(&m_vo_block);
    vorbis_dsp_clear(&m_vo_dsp);
    vorbis_comment_clear(&m_vo_comment);
    vorbis_info_clear(&m_vo_info);
}

void fcVorbisEncoder::release()
{
    delete this;
}

const char* fcVorbisEncoder::getMatroskaCodecID()
{
    return "A_VORBIS";
}

void fcVorbisEncoder::gatherPacketData(fcVorbisFrame& dst)
{
    ogg_page page;
    for (;;) {
        int result = ogg_stream_flush(&m_og_ss, &page);
        if (result == 0) { break; }

        dst.data.append((const char*)page.header, page.header_len);
        dst.data.append((const char*)page.body, page.body_len);

        //if (ogg_page_eos(&page)) { break; }
    }
}

bool fcVorbisEncoder::encode(fcVorbisFrame& dst, const float *samples, size_t num_samples)
{
    if (m_frame == 0) {
        ogg_packet header;
        ogg_packet header_comm;
        ogg_packet header_code;

        vorbis_analysis_headerout(&m_vo_dsp, &m_vo_comment, &header, &header_comm, &header_code);
        ogg_stream_packetin(&m_og_ss, &header);
        ogg_stream_packetin(&m_og_ss, &header_comm);
        ogg_stream_packetin(&m_og_ss, &header_code);
        gatherPacketData(dst);
    }

    int num_channels = m_conf.num_channels;
    int channel_size = (int)num_samples / num_channels;
    float **buffer = vorbis_analysis_buffer(&m_vo_dsp, channel_size);
    for (int bi = 0; bi < channel_size; bi += num_channels) {
        for (int ci = 0; ci < num_channels; ++ci) {
            buffer[ci][bi] = samples[bi*num_channels + ci];
        }
    }
    vorbis_analysis_wrote(&m_vo_dsp, channel_size);

    while (vorbis_analysis_blockout(&m_vo_dsp, &m_vo_block) == 1) {
        vorbis_analysis(&m_vo_block, nullptr);
        vorbis_bitrate_addblock(&m_vo_block);

        ogg_packet packet;
        while (vorbis_bitrate_flushpacket(&m_vo_dsp, &packet)) {
            ogg_stream_packetin(&m_og_ss, &packet);
            gatherPacketData(dst);
        }
    }

    ++m_frame;
    return true;
}

