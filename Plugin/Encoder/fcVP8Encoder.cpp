#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcVPXEncoder.h"

#include "libvpx/vpx/vpx_encoder.h"
#include "libvpx/vpx/vp8cx.h"


class fcVP8Encoder : public fcIVPXEncoder
{
public:
    fcVP8Encoder(const fcVP8EncoderConfig& conf);
    ~fcVP8Encoder() override;
    void release() override;
    bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe) override;
    bool flush(fcVPXFrame& dst) override;

private:
    void gatherFrameData(fcVPXFrame& dst);

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

    // just fill vpx_image_t fields. memory is not needed.
    vpx_img_alloc(&m_vpx_img, VPX_IMG_FMT_I420, conf.width, conf.height, 32);
    vpx_img_free(&m_vpx_img);
    m_vpx_img.img_data = nullptr;
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
    vpx_codec_pts_t vpx_time = uint64_t(timestamp * 1000000.0);
    vpx_enc_frame_flags_t vpx_flags = 0;
    uint32_t duration = uint32_t((timestamp - m_prev_timestamp) * 1000000.0);
    duration = std::max<uint32_t>(duration, 1);
    m_prev_timestamp = timestamp;
    if (force_keyframe) {
        vpx_flags |= VPX_EFLAG_FORCE_KF;
    }

    m_vpx_img.planes[VPX_PLANE_Y] = (uint8_t*)image.y;
    m_vpx_img.planes[VPX_PLANE_U] = (uint8_t*)image.u;
    m_vpx_img.planes[VPX_PLANE_V] = (uint8_t*)image.v;

    auto res = vpx_codec_encode(&m_vpx_ctx, &m_vpx_img, vpx_time, duration, vpx_flags, 0);
    if (res != VPX_CODEC_OK) {
        return false;
    }
    gatherFrameData(dst);

    return true;
}

bool fcVP8Encoder::flush(fcVPXFrame& dst)
{
    auto res = vpx_codec_encode(&m_vpx_ctx, nullptr, -1, 0, 0, 0);
    if (res != VPX_CODEC_OK) {
        return false;
    }
    gatherFrameData(dst);

    return true;
}

void fcVP8Encoder::gatherFrameData(fcVPXFrame& dst)
{
    vpx_codec_iter_t iter = nullptr;
    const vpx_codec_cx_pkt_t *pkt = nullptr;
    while ((pkt = vpx_codec_get_cx_data(&m_vpx_ctx, &iter)) != nullptr) {
        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
            int keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
            dst.timestamp = (double)pkt->data.frame.pts / 1000000.0;
            dst.data.append((char*)pkt->data.frame.buf, pkt->data.frame.sz);
            dst.segments.push_back((int)pkt->data.frame.sz);
        }
    }
}
