#pragma once

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
//#define fcSupportAAC_FAAC
//#ifdef _WIN32
//    #define fcSupportAAC_Intel
//#endif
//#define fcSupportH264_OpenH264
//#ifdef _WIN32
//    #define fcSupportH264_NVIDIA
//    #define fcSupportH264_AMD
//    #define fcSupportH264_Intel
//#endif

#define fcSupportVPX
#define fcSupportVorbis
//#define fcSupportOpus

//#define fcEnableFAACSelfBuild
//#define fcEnableOpenH264Downloader


#ifdef fcEnableLogging
    void DebugLogImpl(const char* fmt, ...);
    #define fcDebugLog(...) DebugLogImpl(__VA_ARGS__)
#else
    #define fcDebugLog(...)
#endif
