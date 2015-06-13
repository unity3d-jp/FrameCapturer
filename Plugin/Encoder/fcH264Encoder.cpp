#include "pch.h"
#include "openh264/codec_api.h"
#include "fcFoundation.h"
#include "fcH264Encoder.h"


#ifdef fcWindows
    #if defined(_M_AMD64)
        #define OpenH264DLL "openh264-1.4.0-win64msvc.dll"
    #elif defined(_M_IX86)
        #define OpenH264DLL "openh264-1.4.0-win32msvc.dll"
    #endif
#else 
#endif

typedef int  (*WelsCreateSVCEncoderT)(ISVCEncoder** ppEncoder);
typedef void (*WelsDestroySVCEncoderT)(ISVCEncoder* pEncoder);

module_t g_h264_dll;
WelsCreateSVCEncoderT g_WelsCreateSVCEncoder = nullptr;
WelsDestroySVCEncoderT g_WelsDestroySVCEncoder = nullptr;

static void LoadOpenH264Module()
{
    if (g_h264_dll != nullptr) { return; }
    g_h264_dll = module_load(OpenH264DLL);
    if (g_h264_dll != nullptr) {
        (void*&)g_WelsCreateSVCEncoder = module_getsymbol(g_h264_dll, "WelsCreateSVCEncoder");
        (void*&)g_WelsDestroySVCEncoder = module_getsymbol(g_h264_dll, "WelsDestroySVCEncoder");
    }
}



fcH264Encoder::fcH264Encoder(int width, int height, float frame_rate, int target_bitrate)
    : m_encoder(nullptr)
    , m_width(width)
    , m_height(height)
{
    LoadOpenH264Module();
    if (g_WelsCreateSVCEncoder) {
        g_WelsCreateSVCEncoder(&m_encoder);

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
}

fcH264Encoder::~fcH264Encoder()
{
    if (g_WelsDestroySVCEncoder)
    {
        g_WelsDestroySVCEncoder(m_encoder);
    }
}

fcH264Encoder::operator bool() const
{
    return m_encoder != nullptr;
}

fcH264Encoder::Result fcH264Encoder::encodeRGBA(const bRGBA *src)
{
    if (!m_encoder) { return Result(); }

    m_buf.resize(roundup<2>(m_width) * roundup<2>(m_height) * 3 / 2);
    uint8_t *pic_y = (uint8_t*)&m_buf[0];
    uint8_t *pic_u = pic_y + (m_width * m_height);
    uint8_t *pic_v = pic_u + ((m_width * m_height) >> 2);
    RGBA_to_I420(pic_y, pic_u, pic_v, src, m_width, m_height);
    return encodeI420(pic_y, pic_u, pic_v);
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
