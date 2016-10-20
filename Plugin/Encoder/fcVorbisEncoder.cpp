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
    fcVorbisEncoderConfig m_conf;

    vorbis_info      m_vo_info;
    vorbis_comment   m_vo_comment;
    vorbis_dsp_state m_vo_dsp;
    vorbis_block     m_vo_block;
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

const char* fcVorbisEncoder::getMatroskaCodecID()
{
    return "A_VORBIS";
}

bool fcVorbisEncoder::encode(fcVorbisFrame& dst, const float *samples, size_t num_samples)
{
    return false;
}

