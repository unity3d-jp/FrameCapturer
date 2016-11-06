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

#define fcSupportOpenGL
#ifdef _WIN32
    #define fcSupportD3D9
    #define fcSupportD3D11
#endif

#define fcSupportPNG
#define fcSupportEXR
#define fcSupportGIF
#define fcSupportMP4
#define fcSupportWebM

#define fcSupportHalfPixelFormat
#define fcSupportAAC_FAAC
#ifdef _WIN32
    #define fcSupportAAC_Intel
#endif
#define fcSupportH264_OpenH264
#ifdef _WIN32
    #define fcSupportH264_NVIDIA
    #define fcSupportH264_AMD
    #define fcSupportH264_Intel
#endif
#define fcSupportVPX
#define fcSupportVorbis
//#define fcSupportOpus

//#define fcEnableFAACSelfBuild
//#define fcEnableOpenH264Downloader


//#define fcGIFSplitModule
//#define fcPNGSplitModule
//#define fcEXRSplitModule
//#define fcMP4SplitModule
//#define fcWebMSplitModule


#ifdef fcEnableLogging
    void DebugLogImpl(const char* fmt, ...);
    #define fcDebugLog(...) DebugLogImpl(__VA_ARGS__)
#else
    #define fcDebugLog(...)
#endif
