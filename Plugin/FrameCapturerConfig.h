#pragma once

#define fcEnableLogging

#define fcSupportOpenGL
#ifdef _WIN32
    #define fcSupportD3D9
    #define fcSupportD3D11

    #define fcSupportPNG
    #define fcSupportEXR
    #define fcSupportGIF
    #define fcSupportMP4
    #define fcSupportWebM

    //// MP4-related encoders
    //#define fcSupportAAC_FAAC
    //#define fcSupportAAC_Intel
    //#define fcSupportH264_OpenH264
    //#define fcSupportH264_NVIDIA
    //#define fcSupportH264_AMD
    //#define fcSupportH264_Intel
    //#define fcEnableFAACSelfBuild
    //#define fcEnableOpenH264Downloader

    //// WebM-related encoders
    #define fcSupportVPX
    #define fcSupportVorbis
    //#define fcSupportOpus
#endif


#ifdef fcEnableLogging
    void DebugLogImpl(const char* fmt, ...);
    #define fcDebugLog(...) DebugLogImpl(__VA_ARGS__)
#else
    #define fcDebugLog(...)
#endif
