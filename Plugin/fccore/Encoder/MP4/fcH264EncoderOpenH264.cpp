#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcSupportH264_OpenH264

#include <openh264/codec_api.h>

#define OpenH264Version "1.6.0"
#if defined(fcWindows)
    #if defined(_M_AMD64)
        #define OpenH264DLL "openh264-" OpenH264Version "-win64msvc.dll"
    #elif defined(_M_IX86)
        #define OpenH264DLL "openh264-" OpenH264Version "-win32msvc.dll"
    #endif
#elif defined(fcMac) 
    #define OpenH264DLL "libopenh264-" OpenH264Version "-osx64.dylib"
#elif defined(fcLinux)
    #define OpenH264DLL "libopenh264-" OpenH264Version "-linux64.3.so"
#endif



class fcH264EncoderOpenH264 : public fcIH264Encoder
{
public:
    fcH264EncoderOpenH264(const fcH264EncoderConfig& conf);
    ~fcH264EncoderOpenH264() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe) override;
    bool flush(fcH264Frame& dst) override;

    bool isValid() const { return m_encoder != nullptr; }

private:
    fcH264EncoderConfig m_conf;
    ISVCEncoder *m_encoder;
    Buffer m_rgba_image;
    I420Image m_i420_image;
};



typedef int  (*WelsCreateSVCEncoder_t)(ISVCEncoder** ppEncoder);
typedef void (*WelsDestroySVCEncoder_t)(ISVCEncoder* pEncoder);

#define EachOpenH264Functions(Body)\
    Body(WelsCreateSVCEncoder)\
    Body(WelsDestroySVCEncoder)

#define Decl(name) static name##_t name##_;
    EachOpenH264Functions(Decl)
#undef Decl

static module_t g_openh264;


bool fcLoadOpenH264Module()
{
    if (g_openh264 != nullptr) { return true; }

    g_openh264 = DLLLoad(OpenH264DLL);
    if (g_openh264 == nullptr) { return false; }

    bool ok = true;
#define Imp(Name) (void*&)Name##_ = DLLGetSymbol(g_openh264, #Name); if(!Name##_) { ok = false; }
    EachOpenH264Functions(Imp)
#undef Imp

    if (!ok) {
        DLLUnload(g_openh264);
        g_openh264 = nullptr;
    }
    return ok;
}


fcH264EncoderOpenH264::fcH264EncoderOpenH264(const fcH264EncoderConfig& conf)
    : m_conf(conf), m_encoder(nullptr)
{
    fcLoadOpenH264Module();
    if (g_openh264 == nullptr) { return; }

    WelsCreateSVCEncoder_(&m_encoder);

    SEncParamBase param;
    memset(&param, 0, sizeof(SEncParamBase));
    param.iUsageType = SCREEN_CONTENT_REAL_TIME;
    param.fMaxFrameRate = (float)conf.target_framerate;
    param.iPicWidth = conf.width;
    param.iPicHeight = conf.height;
    param.iTargetBitrate = conf.target_bitrate;
    param.iRCMode = RC_BITRATE_MODE;
    if (m_encoder->Initialize(&param) != 0) {
        WelsDestroySVCEncoder_(m_encoder);
        m_encoder = nullptr;
    }
}

fcH264EncoderOpenH264::~fcH264EncoderOpenH264()
{
    if (g_openh264 == nullptr) { return; }

    WelsDestroySVCEncoder_(m_encoder);
}

const char* fcH264EncoderOpenH264::getEncoderInfo()
{
    return "OpenH264 Video Codec provided by Cisco Systems, Inc.";
}

bool fcH264EncoderOpenH264::encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool /*force_keyframe*/)
{
    if (!m_encoder) { return false; }

    AnyToI420(m_i420_image, m_rgba_image, image, fmt, m_conf.width, m_conf.height);
    I420Data i420 = m_i420_image.data();

    dst.timestamp = timestamp;

    SSourcePicture src;
    memset(&src, 0, sizeof(src));
    src.iPicWidth = m_conf.width;
    src.iPicHeight = m_conf.height;
    src.iColorFormat = videoFormatI420;
    src.pData[0] = (unsigned char*)i420.y;
    src.pData[1] = (unsigned char*)i420.u;
    src.pData[2] = (unsigned char*)i420.v;
    src.iStride[0] = m_conf.width;
    src.iStride[1] = m_conf.width >> 1;
    src.iStride[2] = m_conf.width >> 1;
    src.uiTimeStamp = to_msec(dst.timestamp);

    SFrameBSInfo frame;
    memset(&frame, 0, sizeof(frame));

    if (m_encoder->EncodeFrame(&src, &frame) != 0) {
        return false;
    }

    {
        dst.type = 0;
        switch (frame.eFrameType) {
        case videoFrameTypeI:   dst.type |= fcH264FrameType_I; break;
        case videoFrameTypeP:   dst.type |= fcH264FrameType_P; break;
        case videoFrameTypeIDR: dst.type |= fcH264FrameType_IDR; break;
        }
    }

    for (int li = 0; li < frame.iLayerNum; ++li) {
        auto& layer = frame.sLayerInfo[li];
        dst.nal_sizes.append(layer.pNalLengthInByte, layer.iNalCount);

        int total = 0;
        for (int ni = 0; ni < layer.iNalCount; ++ni) {
            total += layer.pNalLengthInByte[ni];
        }
        dst.data.append((char*)layer.pBsBuf, total);
    }

    return true;
}

bool fcH264EncoderOpenH264::flush(fcH264Frame& dst)
{
    return false;
}


fcIH264Encoder* fcCreateH264EncoderOpenH264(const fcH264EncoderConfig& conf)
{
    auto *ret = new fcH264EncoderOpenH264(conf);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

#else  // fcSupportH264_OpenH264

bool fcLoadOpenH264Module() { return false; }
fcIH264Encoder* fcCreateH264EncoderOpenH264(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportH264_OpenH264
