#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcVPXEncoder.h"

#include "libvpx/vpx/vpx_encoder.h"
#include "libvpx/vpx/vp8cx.h"

class fcVP9Encoder : public fcIVPXEncoder
{
public:
    fcVP9Encoder(const fcVP9EncoderConfig& conf);
    ~fcVP9Encoder() override;
    void release() override;
    bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe) override;
    bool flush(fcVPXFrame& dst) override;

private:
    fcVP9EncoderConfig m_conf;
    vpx_codec_iface_t *m_vpx_iface = nullptr;
    vpx_codec_ctx_t m_vpx_ctx;
};


fcIVPXEncoder* fcCreateVP9Encoder(const fcVP9EncoderConfig& conf)
{
    return new fcVP9Encoder(conf);
}

fcVP9Encoder::fcVP9Encoder(const fcVP9EncoderConfig& conf)
    : m_conf(conf)
{
    m_vpx_iface = vpx_codec_vp9_cx();

    vpx_codec_enc_cfg_t vpx_config;
    vpx_codec_enc_config_default(m_vpx_iface, &vpx_config, 0);
    vpx_config.g_w = m_conf.width;
    vpx_config.g_h = m_conf.height;
    vpx_config.g_timebase.num = 1;
    vpx_config.g_timebase.den = 1000000;
    vpx_config.rc_target_bitrate = m_conf.target_bitrate;
    vpx_codec_enc_init(&m_vpx_ctx, m_vpx_iface, &vpx_config, 0);
}

fcVP9Encoder::~fcVP9Encoder()
{
}

void fcVP9Encoder::release()
{
    delete this;
}

bool fcVP9Encoder::encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe)
{
    return false;
}

bool fcVP9Encoder::flush(fcVPXFrame& dst)
{
    return false;
}
