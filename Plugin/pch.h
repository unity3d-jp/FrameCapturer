#ifdef _MSC_VER
    #pragma warning(disable: 4190)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <fstream>
#include <sstream>
#include <filesystem>

#define fcImpl

#if defined(_WIN32)
    #define fcWindows
#elif defined(__APPLE__)
    #ifdef TARGET_OS_IPHONE
        #define fciOS
    #else
        #define fcMac
    #endif
#elif defined(__ANDROID__)
    #define fcAndroid
#elif defined(__linux__)
    #define fcLinux
#endif

#define fcEnableLogging
#ifdef _WIN32
    #define fcSupportOpenGL
    #define fcSupportD3D9
    #define fcSupportD3D11

    #define fcSupportPNG
    #define fcSupportEXR
    #define fcSupportGIF
    #define fcSupportMP4
    #define fcSupportWebM

    #define fcSupportHalfPixelFormat
    #define fcSupportFAAC
    #define fcSupportOpenH264
    #define fcSupportNVH264
    #define fcSupportAMDH264

    #define fcSupportVPX
    #define fcSupportVorbis
    //#define fcSupportOpus
#else
    #define fcSupportOpenGL

    #define fcSupportPNG
    #define fcSupportEXR
    #define fcSupportGIF
    #define fcSupportMP4
    #define fcSupportWebM

    #define fcSupportHalfPixelFormat
    #define fcSupportFAAC
    #define fcSupportOpenH264
    #define fcSupportNVH264

    #define fcSupportVPX
    #define fcSupportVorbis
    //#define fcSupportOpus
#endif
#ifndef fcStaticLink
    //#define fcGIFSplitModule
    #define fcPNGSplitModule
    #define fcEXRSplitModule
    #define fcMP4SplitModule
    #define fcWebMSplitModule
#endif // fcStaticLink


#ifdef fcEnableLogging
    void DebugLogImpl(const char* fmt, ...);
    #define fcDebugLog(...) DebugLogImpl(__VA_ARGS__)
#else
    #define fcDebugLog(...)
#endif
