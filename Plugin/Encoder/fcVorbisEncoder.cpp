#include "pch.h"
#include "fcFoundation.h"
#include "fcVorbisEncoder.h"

#ifdef fcSupportVorbis

#include "vorbis/vorbisenc.h"
#ifdef _MSC_VER
    #pragma comment(lib, "libvorbis_static.lib")
    #pragma comment(lib, "libogg_static.lib")
#endif // _MSC_VER


class fcVorbisEncoder : public fcIVorbisEncoder
{
public:
    fcVorbisEncoder(const fcVorbisEncoderConfig& conf);
    ~fcVorbisEncoder() override;
    void release() override;
    const char* getMatroskaCodecID() const override;
    const Buffer& getCodecPrivate() const override;

    bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples, fcTime timestamp) override;
    bool flush(fcVorbisFrame& dst) override;

private:
    void gatherPackets(fcVorbisFrame& dst);

    fcVorbisEncoderConfig   m_conf;
    Buffer                  m_codec_private;

    vorbis_info             m_vo_info;
    vorbis_comment          m_vo_comment;
    vorbis_dsp_state        m_vo_dsp;
    vorbis_block            m_vo_block;
};



fcVorbisEncoder::fcVorbisEncoder(const fcVorbisEncoderConfig& conf)
    : m_conf(conf)
{
    vorbis_info_init(&m_vo_info);
    switch (conf.bitrate_mode) {
    case fcCBR:
        vorbis_encode_init(&m_vo_info, conf.num_channels, conf.sample_rate, conf.target_bitrate, conf.target_bitrate, conf.target_bitrate);
        break;
    case fcVBR:
        vorbis_encode_init(&m_vo_info, conf.num_channels, conf.sample_rate, -1, conf.target_bitrate, -1);
        break;
    }

    vorbis_comment_init(&m_vo_comment);
    vorbis_analysis_init(&m_vo_dsp, &m_vo_info);
    vorbis_block_init(&m_vo_dsp, &m_vo_block);


    {
        // get codec private data
        ogg_packet ident, comment, setup;
        vorbis_analysis_headerout(&m_vo_dsp, &m_vo_comment, &ident, &comment, &setup);

        m_codec_private.resize(ident.bytes + comment.bytes + setup.bytes + 3);
        auto* it = (uint8_t*)m_codec_private.data();
        *it++ = 2;
        *it++ = (uint8_t)(ident.bytes);
        *it++ = (uint8_t)(comment.bytes);
        memcpy(it, ident.packet, ident.bytes);
        it += ident.bytes;
        memcpy(it, comment.packet, comment.bytes);
        it += comment.bytes;
        memcpy(it, setup.packet, setup.bytes);
    }
}

fcVorbisEncoder::~fcVorbisEncoder()
{
    vorbis_block_clear(&m_vo_block);
    vorbis_dsp_clear(&m_vo_dsp);
    vorbis_comment_clear(&m_vo_comment);
    vorbis_info_clear(&m_vo_info);
}

void fcVorbisEncoder::release()
{
    delete this;
}

const char* fcVorbisEncoder::getMatroskaCodecID() const
{
    return "A_VORBIS";
}

const Buffer& fcVorbisEncoder::getCodecPrivate() const
{
    return m_codec_private;
}

void fcVorbisEncoder::gatherPackets(fcVorbisFrame& dst)
{
    while (vorbis_analysis_blockout(&m_vo_dsp, &m_vo_block) == 1) {
        vorbis_analysis(&m_vo_block, nullptr);
        vorbis_bitrate_addblock(&m_vo_block);

        ogg_packet packet;
        while (vorbis_bitrate_flushpacket(&m_vo_dsp, &packet) == 1) {
            dst.data.append((const char*)packet.packet, packet.bytes);

            double timestamp = (double)packet.granulepos / (double)m_conf.sample_rate;
            dst.packets.push_back({ (uint32_t)packet.bytes, 0.0, timestamp });
        }
    }
}

bool fcVorbisEncoder::encode(fcVorbisFrame& dst, const float *samples, size_t num_samples, fcTime timestamp)
{
    if (!samples || num_samples == 0) { return false; }

    int num_channels = m_conf.num_channels;
    int block_size = (int)num_samples / num_channels;
    float **buffer = vorbis_analysis_buffer(&m_vo_dsp, block_size);
    for (int bi = 0; bi < block_size; bi += num_channels) {
        for (int ci = 0; ci < num_channels; ++ci) {
            buffer[ci][bi] = samples[bi*num_channels + ci];
        }
    }

    if (vorbis_analysis_wrote(&m_vo_dsp, block_size) != 0) {
        return false;
    }
    gatherPackets(dst);
    return true;
}

bool fcVorbisEncoder::flush(fcVorbisFrame& dst)
{
    if (vorbis_analysis_wrote(&m_vo_dsp, 0) != 0) {
        return false;
    }
    gatherPackets(dst);
    return true;
}


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf) { return new fcVorbisEncoder(conf); }

#else // fcSupportVorbis

fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf) { return nullptr; }

#endif // fcSupportVorbis
