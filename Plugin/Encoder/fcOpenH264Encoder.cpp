#include "pch.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcSupportOpenH264

#include <openh264/codec_api.h>

#define OpenH264Version "1.5.0"
#ifdef fcWindows
    #if defined(_M_AMD64)
        #define OpenH264URL "http://ciscobinary.openh264.org/openh264-" OpenH264Version "-win64msvc.dll.bz2"
        #define OpenH264DLL "openh264-" OpenH264Version "-win64msvc.dll"
    #elif defined(_M_IX86)
        #define OpenH264URL "http://ciscobinary.openh264.org/openh264-" OpenH264Version "-win32msvc.dll.bz2"
        #define OpenH264DLL "openh264-" OpenH264Version "-win32msvc.dll"
    #endif
#else 
    // Mac
    #define OpenH264URL "http://ciscobinary.openh264.org/libopenh264-" OpenH264Version "-osx64.dylib.bz2"
    #define OpenH264DLL "libopenh264-" OpenH264Version "-osx64.dylib"
#endif



class fcOpenH264Encoder : public fcIH264Encoder
{
public:
    fcOpenH264Encoder(const fcH264EncoderConfig& conf);
    ~fcOpenH264Encoder();
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe) override;

private:
    fcH264EncoderConfig m_conf;
    ISVCEncoder *m_encoder;
};

fcIH264Encoder* fcCreateOpenH264Encoder(const fcH264EncoderConfig& conf)
{
    if (!fcLoadOpenH264Module()) { return nullptr; }
    return new fcOpenH264Encoder(conf);
}


namespace {

typedef int  (*WelsCreateSVCEncoder_t)(ISVCEncoder** ppEncoder);
typedef void (*WelsDestroySVCEncoder_t)(ISVCEncoder* pEncoder);

#define EachOpenH264Functions(Body)\
    Body(WelsCreateSVCEncoder)\
    Body(WelsDestroySVCEncoder)


#define decl(name) name##_t name##_i;
EachOpenH264Functions(decl)
#undef decl

module_t g_mod_h264;

} // namespace


bool fcLoadOpenH264Module()
{
    if (g_mod_h264 != nullptr) { return true; }

    g_mod_h264 = DLLLoad(OpenH264DLL);
    if (g_mod_h264 == nullptr) { return false; }

#define imp(name) (void*&)name##_i = DLLGetSymbol(g_mod_h264, #name);
    EachOpenH264Functions(imp)
#undef imp
    return true;
}


fcOpenH264Encoder::fcOpenH264Encoder(const fcH264EncoderConfig& conf)
    : m_conf(conf), m_encoder(nullptr)
{
    fcLoadOpenH264Module();
    if (g_mod_h264 == nullptr) { return; }

    WelsCreateSVCEncoder_i(&m_encoder);

    SEncParamBase param;
    memset(&param, 0, sizeof(SEncParamBase));
    param.iUsageType = SCREEN_CONTENT_REAL_TIME;
    param.fMaxFrameRate = (float)conf.max_framerate;
    param.iPicWidth = conf.width;
    param.iPicHeight = conf.height;
    param.iTargetBitrate = conf.target_bitrate;
    param.iRCMode = RC_BITRATE_MODE;
    m_encoder->Initialize(&param);
}

fcOpenH264Encoder::~fcOpenH264Encoder()
{
    if (g_mod_h264 == nullptr) { return; }

    WelsDestroySVCEncoder_i(m_encoder);
}
const char* fcOpenH264Encoder::getEncoderInfo() { return "OpenH264 (by Cisco Systems, Inc)"; }

bool fcOpenH264Encoder::encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool /*force_keyframe*/)
{
    if (!m_encoder) { return false; }

    SSourcePicture src;
    memset(&src, 0, sizeof(src));
    src.iPicWidth = m_conf.width;
    src.iPicHeight = m_conf.height;
    src.iColorFormat = videoFormatI420;
    src.pData[0] = (unsigned char*)data.y;
    src.pData[1] = (unsigned char*)data.u;
    src.pData[2] = (unsigned char*)data.v;
    src.iStride[0] = m_conf.width;
    src.iStride[1] = m_conf.width >> 1;
    src.iStride[2] = m_conf.width >> 1;
    src.uiTimeStamp = int64_t(timestamp * 1000.0); // sec to millisec

    SFrameBSInfo frame;
    memset(&frame, 0, sizeof(frame));

    if (m_encoder->EncodeFrame(&src, &frame) != 0) {
        return false;
    }

    dst.h264_type = (fcH264FrameType)frame.eFrameType;

    for (int li = 0; li < frame.iLayerNum; ++li) {
        auto& layer = frame.sLayerInfo[li];
        dst.nal_sizes.append(layer.pNalLengthInByte, layer.iNalCount);

        int total = 0;
        fcH264NALHeader header;
        for (int ni = 0; ni < layer.iNalCount; ++ni) {
            header = fcH264NALHeader(layer.pBsBuf[total + 4]);
            total += layer.pNalLengthInByte[ni];
        }
        dst.data.append((char*)layer.pBsBuf, total);
    }

    return true;
}


// -------------------------------------------------------------
// OpenH264 downloader
// -------------------------------------------------------------


namespace {

    std::thread *g_download_thread;

    std::string fcGetOpenH264ModulePath()
    {
        std::string ret = !fcMP4GetModulePath().empty() ? fcMP4GetModulePath() : DLLGetDirectoryOfCurrentModule();
        if (!ret.empty() && (ret.back() != '/' && ret.back() != '\\')) {
            ret += "/";
        }
        ret += OpenH264DLL;
        return ret;
    }

    void fcDownloadOpenH264Body(fcDownloadCallback cb)
    {
        std::string response;
        if (HTTPGet(OpenH264URL, response)) {
            cb(fcDownloadState_InProgress, "succeeded to HTTP");
            if (BZ2DecompressToFile(fcGetOpenH264ModulePath().c_str(), &response[0], response.size()))
            {
                cb(fcDownloadState_Completed, "succeeded to BZ2 Decompress");
            }
            else {
                cb(fcDownloadState_Error, "failed to BZ2 Decompress");
            }
        }
        else {
            cb(fcDownloadState_Error, "failed to HTTP Get");
        }

        g_download_thread->detach();
        delete g_download_thread;
        g_download_thread = nullptr;
    }

} // namespace

bool fcDownloadOpenH264(fcDownloadCallback cb)
{
    if (fcLoadOpenH264Module()) {
        cb(fcDownloadState_Completed, "file already exists");
        return true;
    }

    // download thread is already running
    if (g_download_thread != nullptr) { return false; }

    g_download_thread = new std::thread([=]() { fcDownloadOpenH264Body(cb); });
    return true;
}

#else  // fcSupportOpenH264

bool fcDownloadOpenH264(fcDownloadCallback cb) { return false; }
bool fcLoadOpenH264Module() { return false; }
fcIH264Encoder* fcCreateOpenH264Encoder(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportOpenH264
