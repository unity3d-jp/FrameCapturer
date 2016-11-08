#include "pch.h"
#include "fcFoundation.h"
#include "fcVPXEncoder.h"

#ifdef fcSupportVPX

#include "libvpx/vpx/vpx_encoder.h"
#include "libvpx/vpx/vp8cx.h"
#ifdef _MSC_VER
    #pragma comment(lib, "vpxmt.lib")
#endif // _MSC_VER


class fcVPXEncoder : public fcIVPXEncoder
{
public:
    fcVPXEncoder(const fcVPXEncoderConfig& conf, fcWebMVideoEncoder encoder);
    ~fcVPXEncoder() override;
    void release() override;
    const char* getMatroskaCodecID() const override;

    bool encode(fcVPXFrame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe) override;
    bool flush(fcVPXFrame& dst) override;

private:
    void gatherFrameData(fcVPXFrame& dst);

    fcVPXEncoderConfig  m_conf = {};
    vpx_codec_iface_t   *m_vpx_iface = nullptr;
    vpx_codec_ctx_t     m_vpx_ctx = {};
    vpx_image_t         m_vpx_img = {};
    const char*         m_matroska_codec_id = nullptr;

    Buffer m_rgba_image;
    I420Image m_i420_image;
};


fcVPXEncoder::fcVPXEncoder(const fcVPXEncoderConfig& conf, fcWebMVideoEncoder encoder)
    : m_conf(conf)
{
    switch (encoder) {
    case fcWebMVideoEncoder::VP8:
        m_matroska_codec_id = "V_VP8";
        m_vpx_iface = vpx_codec_vp8_cx();
        break;
    case fcWebMVideoEncoder::VP9:
        m_matroska_codec_id = "V_VP9";
        m_vpx_iface = vpx_codec_vp9_cx();
        break;
    }

    vpx_codec_enc_cfg_t vpx_config;
    vpx_codec_enc_config_default(m_vpx_iface, &vpx_config, 0);
    vpx_config.g_w = m_conf.width;
    vpx_config.g_h = m_conf.height;
    vpx_config.g_timebase.num = 1;
    vpx_config.g_timebase.den = 1000000000; // nsec
    vpx_config.rc_target_bitrate = m_conf.target_bitrate;
    switch (conf.bitrate_mode) {
    case fcCBR:
        vpx_config.rc_end_usage = VPX_CBR;
        break;
    case fcVBR:
        vpx_config.rc_end_usage = VPX_VBR;
        break;
    }
    vpx_codec_enc_init(&m_vpx_ctx, m_vpx_iface, &vpx_config, 0);

    // just fill vpx_image_t fields. memory is not needed.
    vpx_img_alloc(&m_vpx_img, VPX_IMG_FMT_I420, conf.width, conf.height, 32);
    vpx_img_free(&m_vpx_img);
    m_vpx_img.img_data = nullptr;
}

fcVPXEncoder::~fcVPXEncoder()
{
    vpx_codec_destroy(&m_vpx_ctx);
}

void fcVPXEncoder::release()
{
    delete this;
}

const char* fcVPXEncoder::getMatroskaCodecID() const
{
    return m_matroska_codec_id;
}


bool fcVPXEncoder::encode(fcVPXFrame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe)
{
    AnyToI420(m_i420_image, m_rgba_image, image, fmt, m_conf.width, m_conf.height);
    auto data = m_i420_image.data();

    vpx_codec_pts_t vpx_time = to_nsec(timestamp);
    vpx_enc_frame_flags_t vpx_flags = 0;
    uint32_t duration = 1000000000 / m_conf.target_framerate;
    if (force_keyframe) {
        vpx_flags |= VPX_EFLAG_FORCE_KF;
    }

    m_vpx_img.planes[VPX_PLANE_Y] = (uint8_t*)data.y;
    m_vpx_img.planes[VPX_PLANE_U] = (uint8_t*)data.u;
    m_vpx_img.planes[VPX_PLANE_V] = (uint8_t*)data.v;

    auto res = vpx_codec_encode(&m_vpx_ctx, &m_vpx_img, vpx_time, duration, vpx_flags, 0);
    if (res != VPX_CODEC_OK) {
        return false;
    }
    gatherFrameData(dst);

    return true;
}

bool fcVPXEncoder::flush(fcVPXFrame& dst)
{
    auto res = vpx_codec_encode(&m_vpx_ctx, nullptr, -1, 0, 0, 0);
    if (res != VPX_CODEC_OK) {
        return false;
    }
    gatherFrameData(dst);

    return true;
}

void fcVPXEncoder::gatherFrameData(fcVPXFrame& dst)
{
    vpx_codec_iter_t iter = nullptr;
    const vpx_codec_cx_pkt_t *pkt = nullptr;
    while ((pkt = vpx_codec_get_cx_data(&m_vpx_ctx, &iter)) != nullptr) {
        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
            dst.data.append((char*)pkt->data.frame.buf, pkt->data.frame.sz);

            double timestamp = nsec_to_sec(pkt->data.frame.pts);
            dst.packets.push_back({
                (int)pkt->data.frame.sz, timestamp, pkt->data.frame.flags & VPX_FRAME_IS_KEY });
        }
    }
}


fcIVPXEncoder* fcCreateVP8EncoderLibVPX(const fcVPXEncoderConfig& conf) { return new fcVPXEncoder(conf, fcWebMVideoEncoder::VP8); }
fcIVPXEncoder* fcCreateVP9EncoderLibVPX(const fcVPXEncoderConfig& conf) { return new fcVPXEncoder(conf, fcWebMVideoEncoder::VP9); }

#else // fcSupportVPX

fcIVPXEncoder* fcCreateVP8EncoderLibVPX(const fcVPXEncoderConfig& conf) { return nullptr; }
fcIVPXEncoder* fcCreateVP9EncoderLibVPX(const fcVPXEncoderConfig& conf) { return nullptr; }

#endif // fcSupportVPX
