#include "pch.h"
#include <openh264/codec_api.h>
#include <libyuv/libyuv.h>
#include "fcFoundation.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcWindows
    #pragma comment(lib, "libbz2.lib")
    #pragma comment(lib, "libcurl.lib")
    #pragma comment(lib, "ws2_32.lib")
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

    g_mod_h264 = DLLLoad(OpenH264DLL);
    if (g_mod_h264 == nullptr) { return false; }

#define imp(name) (void*&)name##_imp = DLLGetSymbol(g_mod_h264, #name);
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
    param.iUsageType = SCREEN_CONTENT_REAL_TIME;
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


bool fcH264Encoder::encodeI420(fcH264Frame& dst, const void *src_y, const void *src_u, const void *src_v, uint64_t timestamp)
{
    if (!m_encoder) { return false; }

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
    src.uiTimeStamp = timestamp / 1000000; // nanosec to millisec

    SFrameBSInfo frame;
    memset(&frame, 0, sizeof(frame));

    if (m_encoder->EncodeFrame(&src, &frame) != 0) {
        return false;
    }

    dst.nal_sizes.clear();
    dst.data.clear();
    dst.h264_type = (fcH264FrameType)frame.eFrameType;

    for (int li = 0; li < frame.iLayerNum; ++li) {
        auto& layer = frame.sLayerInfo[li];
        dst.nal_sizes.assign(layer.pNalLengthInByte, layer.pNalLengthInByte + layer.iNalCount);

        int total = 0;
        for (int ni = 0; ni < layer.iNalCount; ++ni) {
            total += layer.pNalLengthInByte[ni];
        }
        dst.data.append(layer.pBsBuf, total);
    }

    return true;
}
