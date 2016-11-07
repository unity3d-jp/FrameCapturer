#include "pch.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcSupportH264_Intel

#include "IntelQSV/mfxvideo++.h"
#ifdef _WIN32
    #pragma comment(lib, "libmfx.lib")
    #pragma comment(lib, "legacy_stdio_definitions.lib")
#endif


class fcH264EncoderIntel : public fcIH264Encoder
{
public:
    fcH264EncoderIntel(const fcH264EncoderConfig& conf);
    ~fcH264EncoderIntel() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe) override;

    bool isValid() { return m_encoder != nullptr; }

private:
    fcH264EncoderConfig m_conf;
    std::unique_ptr<MFXVideoSession> m_session;
    std::unique_ptr<MFXVideoENCODE> m_encoder;
    mfxEncodeCtrl m_econtrol;
    mfxFrameSurface1 m_surface;
    mfxBitstream m_bitstream;
    mfxSyncPoint m_sync;
};



fcH264EncoderIntel::fcH264EncoderIntel(const fcH264EncoderConfig& conf)
    : m_conf(conf)
{
    mfxStatus ret;
    m_session.reset(new MFXVideoSession());
    ret = m_session->Init(MFX_IMPL_AUTO_ANY, nullptr);
    if (ret < 0) {
        m_session.reset();
        return;
    }

    mfxVideoParam params;
    memset(&params, 0, sizeof(params));

    auto& fi = params.mfx.FrameInfo;
    fi.FourCC = MFX_FOURCC_NV12;
    fi.Width = roundup<2>(conf.width);
    fi.Height = roundup<2>(conf.height);
    fi.CropX = 0;
    fi.CropY = 0;
    fi.CropW = conf.width;
    fi.CropH = conf.height;
    fi.FrameRateExtN = conf.target_framerate * 1000000;
    fi.FrameRateExtD = 1000000;
    fi.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    fi.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    params.mfx.CodecId = MFX_CODEC_AVC;
    params.mfx.CodecProfile = MFX_PROFILE_AVC_MAIN;
    params.mfx.GopOptFlag = MFX_GOP_CLOSED;
    params.mfx.GopPicSize = 0;
    params.mfx.GopRefDist = 0;
    params.mfx.TargetUsage = MFX_TARGETUSAGE_BEST_SPEED;

    params.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
    params.mfx.TargetKbps = conf.target_bitrate / 1000;
    params.mfx.MaxKbps = 0;
    params.mfx.BufferSizeInKB = (fi.Width * fi.Height * 2) / 1024;
    params.mfx.NumSlice = 0;
    params.mfx.NumRefFrame = 0;

    params.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

    m_encoder.reset(new MFXVideoENCODE(*m_session));
    ret = m_encoder->Init(&params);
    if (ret < 0) {
        m_encoder.reset();
    }


    memset(&m_econtrol, 0, sizeof(m_econtrol));
    memset(&m_surface, 0, sizeof(m_surface));
    memset(&m_bitstream, 0, sizeof(m_bitstream));
}

fcH264EncoderIntel::~fcH264EncoderIntel()
{
    m_encoder.reset();
    m_session.reset();
}

const char* fcH264EncoderIntel::getEncoderInfo() { return "Intel H264 Encoder"; }

bool fcH264EncoderIntel::encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe)
{
    if (!isValid()) { return false; }

    m_surface.Data.TimeStamp = (mfxU64)(timestamp * 90000.0); // unit is 90KHz
    m_surface.Data.MemType = MFX_MEMTYPE_SYSTEM_MEMORY;
    m_surface.Data.Y = (mfxU8*)data.y;
    m_surface.Data.U = (mfxU8*)data.u;
    m_surface.Data.V = (mfxU8*)data.v;

    mfxStatus ret = m_encoder->EncodeFrameAsync(&m_econtrol, &m_surface, &m_bitstream, &m_sync);
    if (ret < 0) { return false; }

    m_surface.Data.FrameOrder++;

    return true;
}

fcIH264Encoder* fcCreateH264EncoderIntel(const fcH264EncoderConfig& conf)
{
    auto ret = new fcH264EncoderIntel(conf);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

#else // fcSupportH264_Intel

fcIH264Encoder* fcCreateH264EncoderIntel(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportH264_Intel
