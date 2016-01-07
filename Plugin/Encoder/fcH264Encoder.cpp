#include "pch.h"
#define CURL_STATICLIB
#include <curl/curl.h>
#include <openh264/codec_api.h>
#include <libyuv/libyuv.h>
#include <bzip2/bzlib.h>
#include "fcFoundation.h"
#include "fcH264Encoder.h"

#ifdef fcWindows
    #pragma comment(lib, "libbz2.lib")
    #pragma comment(lib, "libcurl.lib")
    #pragma comment(lib, "ws2_32.lib")

    #if defined(_M_AMD64)
        #define OpenH264URL "http://ciscobinary.openh264.org/openh264-1.5.0-win64msvc.dll.bz2"
        #define OpenH264DLL "openh264-1.5.0-win64msvc.dll"
    #elif defined(_M_IX86)
        #define OpenH264URL "http://ciscobinary.openh264.org/openh264-1.5.0-win32msvc.dll.bz2"
        #define OpenH264DLL "openh264-1.5.0-win32msvc.dll"
    #endif
#else 
    // Mac
    #define OpenH264URL "http://ciscobinary.openh264.org/libopenh264-1.5.0-osx64.dylib.bz2"
    #define OpenH264DLL "libopenh264-1.5.0-osx64.dylib"
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



static bool fcBZ2MemoryToFile(const char *src, size_t src_len, const char *dst_path)
{
    std::vector<char> buf(1024 * 1024);

    unsigned int dst_len = buf.size();
    int ret = BZ2_bzBuffToBuffDecompress(&buf[0], &dst_len, (char*)src, src_len, 0, 0);
    if (ret == BZ_OK) {
        FILE *fout = fopen(dst_path, "wb");
        if (fout == nullptr) { return false; }
        fwrite(&buf[0], 1, dst_len, fout);
        fclose(fout);
        return true;
    }
    return false;
}

static int fcHTTPCalback(char* data, size_t size, size_t nmemb, std::string *response)
{
    size_t len = size * nmemb;
    response->append(data, len);
    return (int)len;

}

static bool fcHTTPGet(const char *url, std::string &response)
{
    CURL *curl = curl_easy_init();
    if (curl == nullptr) { return false; }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fcHTTPCalback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    bool ret = true;
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        ret = false;
        response = curl_easy_strerror(res);
    }
    curl_easy_cleanup(curl);
    return ret;
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
    if (fcHTTPGet(OpenH264URL, response)) {
        cb(false, "HTTP Get completed");
        if (fcBZ2MemoryToFile(&response[0], response.size(), path_to_dll.c_str())) {
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
