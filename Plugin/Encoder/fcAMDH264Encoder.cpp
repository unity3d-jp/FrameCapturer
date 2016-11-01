#include "pch.h"
#include <libyuv/libyuv.h>
#include "fcFoundation.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcSupportAMDH264

#if defined(_M_AMD64)
    #define AmfCoreDll  "amf-core-windesktop64.dll"
    #define AmfVCEDll   "amf-component-vce-windesktop64.dll"
#elif defined(_M_IX86)
    #define AmfCoreDll  "amf-core-windesktop32.dll"
    #define AmfVCEDll   "amf-component-vce-windesktop32.dll"
#endif


class fcAMDH264Encoder : public fcIH264Encoder
{
public:
    fcAMDH264Encoder(const fcH264EncoderConfig& conf);
    ~fcAMDH264Encoder();
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe) override;

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


fcAMDH264Encoder::fcAMDH264Encoder(const fcH264EncoderConfig& conf)
    : m_conf(conf)
{
}

fcAMDH264Encoder::~fcAMDH264Encoder()
{
}
const char* fcAMDH264Encoder::getEncoderInfo() { return "VCE (by AMD)"; }

bool fcAMDH264Encoder::encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe)
{
    return false;
}

fcIH264Encoder* fcCreateAMDH264Encoder(const fcH264EncoderConfig& conf)
{
    return nullptr; // until fcAMDH264Encoder is implemented properly

    if (!fcLoadAMDH264Module()) { return nullptr; }
    return new fcAMDH264Encoder(conf);
}

#else  // fcSupportAMDH264

fcIH264Encoder* fcCreateAMDH264Encoder(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportAMDH264
