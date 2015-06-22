#include "pch.h"
#include <openh264/codec_api.h>
#include <libyuv/libyuv.h>
#include "fcFoundation.h"
#include "fcH264Encoder.h"


#ifdef fcWindows
    #if defined(_M_AMD64)
        #define OpenH264DLL "openh264-1.4.0-win64msvc.dll"
    #elif defined(_M_IX86)
        #define OpenH264DLL "openh264-1.4.0-win32msvc.dll"
    #endif
#else 
#define "libopenh264-1.4.0-osx64.dylib"
#endif

namespace {

typedef int  (*WelsCreateSVCEncoder_t)(ISVCEncoder** ppEncoder);
typedef void (*WelsDestroySVCEncoder_t)(ISVCEncoder* pEncoder);

#define decl(name) name##_t name##_imp;
decl(WelsCreateSVCEncoder)
decl(WelsDestroySVCEncoder)
#undef decl

module_t g_mod_h264;

static bool LoadOpenH264Module()
{
    if (g_mod_h264 != nullptr) { return true; }

    g_mod_h264 = module_load(OpenH264DLL);
    if (g_mod_h264 == nullptr) { return false; }

#define imp(name) (void*&)name##_imp = module_getsymbol(g_mod_h264, #name);
    imp(WelsCreateSVCEncoder)
    imp(WelsDestroySVCEncoder)
#undef imp
    return true;
}

} // namespace



bool fcH264Encoder::loadModule()
{
    return LoadOpenH264Module();
}

fcH264Encoder::fcH264Encoder(int width, int height, float frame_rate, int target_bitrate)
    : m_encoder(nullptr)
    , m_width(width)
    , m_height(height)
{
    loadModule();
    if (g_mod_h264 == nullptr) { return; }

    WelsCreateSVCEncoder_imp(&m_encoder);

    SEncParamBase param;
    memset(&param, 0, sizeof(SEncParamBase));
    param.iUsageType = CAMERA_VIDEO_REAL_TIME;
    param.fMaxFrameRate = frame_rate;
    param.iPicWidth = m_width;
    param.iPicHeight = m_height;
    param.iTargetBitrate = target_bitrate;
    param.iRCMode = RC_BITRATE_MODE;
    int ret = m_encoder->Initialize(&param);
}

fcH264Encoder::~fcH264Encoder()
{
    if (g_mod_h264 == nullptr) { return; }

    WelsDestroySVCEncoder_imp(m_encoder);
}

fcH264Encoder::operator bool() const
{
    return m_encoder != nullptr;
}


fcH264Encoder::Result fcH264Encoder::encodeI420(const void *src_y, const void *src_u, const void *src_v)
{
    if (!m_encoder) { return Result(); }

    SSourcePicture src;
    memset(&src, 0, sizeof(src));
    src.iPicWidth = m_width;
    src.iPicHeight = m_height;
    src.iColorFormat = videoFormatI420;
    src.pData[0] = (unsigned char*)src_y;
    src.pData[1] = (unsigned char*)src_u;
    src.pData[2] = (unsigned char*)src_v;
    src.iStride[0] = m_width;
    src.iStride[1] = m_width >> 1;
    src.iStride[2] = m_width >> 1;

    SFrameBSInfo dst;
    memset(&dst, 0, sizeof(dst));

    int ret = m_encoder->EncodeFrame(&src, &dst);
    if (ret == 0) {
        return Result(
            dst.sLayerInfo[0].pBsBuf,
            dst.iFrameSizeInBytes,
            (FrameType)dst.eFrameType);
    }
    return Result();
}
