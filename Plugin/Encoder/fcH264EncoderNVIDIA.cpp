#include "pch.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"
#include "NVENC/nvEncodeAPI.h"

#ifdef fcSupportH264_NVIDIA

#ifdef _WIN32
    #if defined(_M_AMD64)
        #define NVEncoderDLL    "nvEncodeAPI64.dll"
    #elif defined(_M_IX86)
        #define NVEncoderDLL    "nvEncodeAPI.dll"
    #endif
#else
    #define NVEncoderDLL    "libnvidia-encode.so.1"
#endif


class fcH264EncoderNVIDIA : public fcIH264Encoder
{
public:
    fcH264EncoderNVIDIA(const fcH264EncoderConfig& conf);
    ~fcH264EncoderNVIDIA() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe) override;

private:
    fcH264EncoderConfig m_conf;
};



static module_t g_mod_h264nv;

static bool LoadNVENCModule()
{
    if (g_mod_h264nv) { return true; }

    g_mod_h264nv = DLLLoad(NVEncoderDLL);
    return g_mod_h264nv != nullptr;
}


fcH264EncoderNVIDIA::fcH264EncoderNVIDIA(const fcH264EncoderConfig& conf)
    : m_conf(conf)
{
}

fcH264EncoderNVIDIA::~fcH264EncoderNVIDIA()
{
}
const char* fcH264EncoderNVIDIA::getEncoderInfo() { return "NVIDIA H264 Encoder"; }

bool fcH264EncoderNVIDIA::encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe)
{
    return false;
}

fcIH264Encoder* fcCreateH264EncoderNVIDIA(const fcH264EncoderConfig& conf)
{
    return nullptr; // until fcNVH264Encoder is implemented properly

    if (!LoadNVENCModule()) { return nullptr; }
    return new fcH264EncoderNVIDIA(conf);
}

#else // fcSupportH264_NVIDIA

fcIH264Encoder* fcCreateH264EncoderNVIDIA(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportH264_NVIDIA
