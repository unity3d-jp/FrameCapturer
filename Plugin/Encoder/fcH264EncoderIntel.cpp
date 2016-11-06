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
    MFXVideoSession m_session;
    std::unique_ptr<MFXVideoENCODE> m_encoder;
};



fcH264EncoderIntel::fcH264EncoderIntel(const fcH264EncoderConfig& conf)
    : m_conf(conf)
{
    mfxStatus ret;
    ret = m_session.Init(MFX_IMPL_AUTO_ANY, nullptr);
    if (ret < 0) {
        return;
    }

    mfxVideoParam params;
    memset(&params, 0, sizeof(params));
    params.mfx.CodecId = MFX_CODEC_AVC;
    params.mfx.CodecProfile = MFX_PROFILE_AVC_MAIN;
    params.mfx.GopOptFlag = MFX_GOP_CLOSED;
    params.mfx.GopPicSize = 0;
    params.mfx.GopRefDist = 0;
    params.mfx.TargetUsage = MFX_TARGETUSAGE_BEST_SPEED;

    params.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
    params.mfx.TargetKbps = conf.target_bitrate / 1000;
    params.mfx.MaxKbps = 0;
    params.mfx.BufferSizeInKB = conf.width * conf.height * 2;
    params.mfx.NumSlice = 0;
    params.mfx.NumRefFrame = 0;

    params.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

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

    m_encoder.reset(new MFXVideoENCODE(m_session));
    ret = m_encoder->Init(&params);
    if (ret < 0) {
        m_encoder.reset();
    }
}

fcH264EncoderIntel::~fcH264EncoderIntel()
{
    m_encoder.reset();
    m_session.Close();
}

const char* fcH264EncoderIntel::getEncoderInfo() { return "Intel H264 Encoder"; }

bool fcH264EncoderIntel::encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe)
{
    return false;
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
