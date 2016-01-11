#include "pch.h"
#include <openh264/codec_api.h>
#include <libyuv/libyuv.h>
#include "fcFoundation.h"
#include "fcH264Encoder.h"

#define OpenH264Version "1.5.0"

#ifdef fcWindows
    #pragma comment(lib, "libbz2.lib")
    #pragma comment(lib, "libcurl.lib")
    #pragma comment(lib, "ws2_32.lib")

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


static const std::string& fcGetPathOfThisModule()
{
    static std::string s_path;

    if (s_path.empty()) {
        char buf[MAX_PATH];
#ifdef fcWindows
        HMODULE mod = 0;
        ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&fcGetPathOfThisModule, &mod);
        DWORD size = ::GetModuleFileNameA(mod, buf, sizeof(buf));
        for (int i = size - 1; i >= 0; --i) {
            if (buf[i] == '\\') {
                buf[i] = '\0';
                s_path = buf;
                break;
            }
        }
#else
        // todo
#endif
    }
    return s_path;
}



static std::thread *g_download_thread;

static void fcDummyDownloadCB(bool, const char*)
{
}

static void fcMP4DownloadCodecBody(fcDownloadCallback cb)
{
    if (cb == nullptr) { cb = &fcDummyDownloadCB; }

    std::string path_to_dll = fcGetPathOfThisModule() + "/" OpenH264DLL;
    std::string response;
    if (HTTPGet(OpenH264URL, response)) {
        cb(false, "HTTP Get completed");
        if (BZ2DecompressToFile(path_to_dll.c_str(), &response[0], response.size())) {
            cb(true, "BZ2 Decompress completed");
        }
        else {
            cb(true, "BZ2 Decompress failed");
        }
    }
    else {
        cb(true, "HTTP Get failed");
    }

    g_download_thread->detach();
    delete g_download_thread;
    g_download_thread = nullptr;
}

fcCLinkage fcExport bool fcMP4DownloadCodecImpl(fcDownloadCallback cb)
{
    if (g_download_thread != nullptr) { return false; }

    std::string path_to_dll = fcGetPathOfThisModule() + "/" OpenH264DLL;
    if (FILE *file = fopen(path_to_dll.c_str(), "r")) {
        fclose(file);
        return false;
    }

    g_download_thread = new std::thread([=]() { fcMP4DownloadCodecBody(cb); });
    return true;
}


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


fcH264Encoder::FrameData fcH264Encoder::encodeI420(const void *src_y, const void *src_u, const void *src_v)
{
    if (!m_encoder) { return FrameData(); }

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
        return FrameData(
            dst.sLayerInfo[0].pBsBuf,
            dst.iFrameSizeInBytes,
            (FrameType)dst.eFrameType);
    }
    return FrameData();
}
