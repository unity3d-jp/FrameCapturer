#include "pch.h"
#include "fcInternal.h"

#ifdef fcSupportWebM
#include "fcWebMInternal.h"
#include "fcVPXEncoder.h"

#ifdef fcSupportVPX
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#ifdef _MSC_VER
    #pragma comment(lib, "vpxmt.lib")
#endif // _MSC_VER


class fcVPXEncoder : public fcIWebMVideoEncoder
{
public:
    fcVPXEncoder(const fcVPXEncoderConfig& conf, fcWebMVideoEncoder encoder);
    ~fcVPXEncoder() override;
    const char* getMatroskaCodecID() const override;
    const Buffer& getCodecPrivate() const override;

    bool encode(fcWebMFrameData& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe) override;
    bool flush(fcWebMFrameData& dst) override;

private:
    void gatherFrameData(fcWebMFrameData& dst);

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
    case fcWebMVideoEncoder::VPX_VP8:
        m_matroska_codec_id = "V_VP8";
        m_vpx_iface = vpx_codec_vp8_cx();
        break;
    case fcWebMVideoEncoder::VPX_VP9:
    case fcWebMVideoEncoder::VPX_VP9LossLess:
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

    if (encoder != fcWebMVideoEncoder::VPX_VP9LossLess) {
        switch (conf.bitrate_mode) {
        case fcBitrateMode::CBR:
            vpx_config.rc_end_usage = VPX_CBR;
            break;
        case fcBitrateMode::VBR:
            vpx_config.rc_end_usage = VPX_VBR;
            break;
        }
    }
    vpx_codec_enc_init(&m_vpx_ctx, m_vpx_iface, &vpx_config, 0);
    if (encoder == fcWebMVideoEncoder::VPX_VP9LossLess) {
        vpx_codec_control_(&m_vpx_ctx, VP9E_SET_LOSSLESS, 1);
    }

    vpx_img_wrap(&m_vpx_img, VPX_IMG_FMT_I420, m_conf.width, m_conf.height, 2, nullptr);
}

fcVPXEncoder::~fcVPXEncoder()
{
    vpx_codec_destroy(&m_vpx_ctx);
}

const char* fcVPXEncoder::getMatroskaCodecID() const
{
    return m_matroska_codec_id;
}

const Buffer & fcVPXEncoder::getCodecPrivate() const
{
    static Buffer s_dummy;
    return s_dummy;
}


bool fcVPXEncoder::encode(fcWebMFrameData& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe)
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

bool fcVPXEncoder::flush(fcWebMFrameData& dst)
{
    auto res = vpx_codec_encode(&m_vpx_ctx, nullptr, -1, 0, 0, 0);
    if (res != VPX_CODEC_OK) {
        return false;
    }
    gatherFrameData(dst);

    return true;
}

void fcVPXEncoder::gatherFrameData(fcWebMFrameData& dst)
{
    vpx_codec_iter_t iter = nullptr;
    const vpx_codec_cx_pkt_t *pkt = nullptr;
    while ((pkt = vpx_codec_get_cx_data(&m_vpx_ctx, &iter)) != nullptr) {
        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
            dst.data.append((char*)pkt->data.frame.buf, pkt->data.frame.sz);

            double timestamp = nsec_to_sec(pkt->data.frame.pts);
            dst.packets.push_back({
                (uint32_t)pkt->data.frame.sz, timestamp, (uint32_t)(pkt->data.frame.flags & VPX_FRAME_IS_KEY) });
        }
    }
}


fcIWebMVideoEncoder* fcCreateVPXVP8Encoder(const fcVPXEncoderConfig& conf) { return new fcVPXEncoder(conf, fcWebMVideoEncoder::VPX_VP8); }
fcIWebMVideoEncoder* fcCreateVPXVP9Encoder(const fcVPXEncoderConfig& conf) { return new fcVPXEncoder(conf, fcWebMVideoEncoder::VPX_VP9); }
fcIWebMVideoEncoder* fcCreateVPXVP9LossLessEncoder(const fcVPXEncoderConfig& conf) { return new fcVPXEncoder(conf, fcWebMVideoEncoder::VPX_VP9LossLess); }

#else // fcSupportVPX

fcIWebMVideoEncoder* fcCreateVPXVP8Encoder(const fcVPXEncoderConfig& conf) { return nullptr; }
fcIWebMVideoEncoder* fcCreateVPXVP9Encoder(const fcVPXEncoderConfig& conf) { return nullptr; }
fcIWebMVideoEncoder* fcCreateVPXVP9LossLessEncoder(const fcVPXEncoderConfig& conf) { return nullptr; }

#endif // fcSupportVPX
#endif // fcSupportWebM
