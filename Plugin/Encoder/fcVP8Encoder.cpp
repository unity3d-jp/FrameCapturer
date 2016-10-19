#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcVPXEncoder.h"

#include "libvpx/vpx/vpx_encoder.h"
#include "libvpx/vpx/vp8cx.h"

struct VP8VideoFrame
{
    fcTime timestamp;
    Buffer data;
};

class fcVP8Encoder : public fcIVPXEncoder
{
public:
    fcVP8Encoder(const fcVP8EncoderConfig& conf);
    ~fcVP8Encoder() override;
    void release() override;
    bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe) override;

private:
    fcVP8EncoderConfig  m_conf = {};
    vpx_codec_ctx_t     m_vpx_ctx = {};
    vpx_codec_iface_t   *m_vpx_iface = nullptr;
    vpx_image_t         m_vpx_img = {};
    fcTime              m_prev_timestamp = 0.0;
};


fcIVPXEncoder* fcCreateVP8Encoder(const fcVP8EncoderConfig& conf)
{
    return new fcVP8Encoder(conf);
}

fcVP8Encoder::fcVP8Encoder(const fcVP8EncoderConfig& conf)
    : m_conf(conf)
{
    m_vpx_iface = vpx_codec_vp8_cx();

    vpx_codec_enc_cfg_t vpx_config;
    vpx_codec_enc_config_default(m_vpx_iface, &vpx_config, 0);
    vpx_config.g_w = m_conf.width;
    vpx_config.g_h = m_conf.height;
    vpx_config.g_timebase.num = 1;
    vpx_config.g_timebase.den = 1000000;
    vpx_config.rc_target_bitrate = m_conf.target_bitrate;
    vpx_codec_enc_init(&m_vpx_ctx, m_vpx_iface, &vpx_config, 0);

    m_vpx_img.fmt = VPX_IMG_FMT_I420;
    vpx_img_set_rect(&m_vpx_img, 0, 0, conf.width, conf.height);
}

fcVP8Encoder::~fcVP8Encoder()
{
    vpx_codec_destroy(&m_vpx_ctx);
}

void fcVP8Encoder::release()
{
    delete this;
}

bool fcVP8Encoder::encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe)
{
    vpx_image_t vpx_img;
    vpx_codec_pts_t vpx_time = uint64_t(timestamp * 1000000.0);
    vpx_enc_frame_flags_t vpx_flags = 0;
    uint32_t duration = uint32_t((timestamp - m_prev_timestamp) * 1000000.0);
    m_prev_timestamp = timestamp;
    if (force_keyframe) {
        vpx_flags |= VPX_EFLAG_FORCE_KF;
    }

    auto res = vpx_codec_encode(&m_vpx_ctx, &vpx_img, vpx_time, duration, vpx_flags, 0);
    if (res != VPX_CODEC_OK) {
        return false;
    }

    vpx_codec_iter_t iter = nullptr;
    const vpx_codec_cx_pkt_t *pkt = nullptr;
    while ((pkt = vpx_codec_get_cx_data(&m_vpx_ctx, &iter)) != nullptr) {
        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
            int keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
            dst.timestamp = timestamp;
            dst.data.append((char*)pkt->data.frame.buf, pkt->data.frame.sz);
            dst.segments.push_back((int)pkt->data.frame.sz);
        }
    }

    return true;
}
