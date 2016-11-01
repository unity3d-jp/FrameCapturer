#include "pch.h"
#include <libyuv/libyuv.h>
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcSupportNVH264

#ifdef _WIN32
    #if defined(_M_AMD64)
        #define NVEncoderDLL    "nvEncodeAPI64.dll"
    #elif defined(_M_IX86)
        #define NVEncoderDLL    "nvEncodeAPI.dll"
    #endif
#else
    #define NVEncoderDLL    "libnvidia-encode.so.1"
#endif


class fcNVH264Encoder : public fcIH264Encoder
{
public:
    fcNVH264Encoder(const fcH264EncoderConfig& conf);
    ~fcNVH264Encoder();
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe) override;

private:
    fcH264EncoderConfig m_conf;
};



namespace {
    module_t g_nvencoder;
} // namespace

bool fcLoadNVH264Module()
{
    if (g_nvencoder) { return true; }

    g_nvencoder = DLLLoad(NVEncoderDLL);
    return g_nvencoder != nullptr;
}


fcNVH264Encoder::fcNVH264Encoder(const fcH264EncoderConfig& conf)
    : m_conf(conf)
{
}

fcNVH264Encoder::~fcNVH264Encoder()
{
}
const char* fcNVH264Encoder::getEncoderInfo() { return "NVENC (by NVIDIA)"; }

bool fcNVH264Encoder::encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe)
{
    return false;
}

fcIH264Encoder* fcCreateNVH264Encoder(const fcH264EncoderConfig& conf)
{
    return nullptr; // until fcNVH264Encoder is implemented properly

    if (!fcLoadNVH264Module()) { return nullptr; }
    return new fcNVH264Encoder(conf);
}

#else // fcSupportNVH264

fcIH264Encoder* fcCreateNVH264Encoder(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportNVH264
