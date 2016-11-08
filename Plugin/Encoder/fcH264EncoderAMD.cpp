#include "pch.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcSupportH264_AMD

#if defined(_M_AMD64)
    #define AmfCoreDll  "amf-core-windesktop64.dll"
    #define AmfVCEDll   "amf-component-vce-windesktop64.dll"
#elif defined(_M_IX86)
    #define AmfCoreDll  "amf-core-windesktop32.dll"
    #define AmfVCEDll   "amf-component-vce-windesktop32.dll"
#endif


class fcH264EncoderAMD : public fcIH264Encoder
{
public:
    fcH264EncoderAMD(const fcH264EncoderConfig& conf);
    ~fcH264EncoderAMD() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe) override;

private:
    fcH264EncoderConfig m_conf;
};



namespace {
    module_t g_amf_core;
    module_t g_amf_vce;
} // namespace

bool fcLoadAMDH264Module()
{
    if (g_amf_core && g_amf_vce) { return true; }

    g_amf_core = DLLLoad(AmfCoreDll);
    g_amf_vce = DLLLoad(AmfVCEDll);
    return g_amf_core && g_amf_vce;
}


fcH264EncoderAMD::fcH264EncoderAMD(const fcH264EncoderConfig& conf)
    : m_conf(conf)
{
}

fcH264EncoderAMD::~fcH264EncoderAMD()
{
}
const char* fcH264EncoderAMD::getEncoderInfo() { return "AMD H264 Encoder"; }

bool fcH264EncoderAMD::encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe)
{
    return false;
}

fcIH264Encoder* fcCreateH264EncoderAMD(const fcH264EncoderConfig& conf)
{
    return nullptr; // until fcAMDH264Encoder is implemented properly

    if (!fcLoadAMDH264Module()) { return nullptr; }
    return new fcH264EncoderAMD(conf);
}

#else  // fcSupportH264_AMD

fcIH264Encoder* fcCreateH264EncoderAMD(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportH264_AMD
