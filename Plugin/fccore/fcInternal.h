#pragma once

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


#ifdef _WIN32
    #define fcSupportOpenGL
    #define fcSupportD3D9
    #define fcSupportD3D11

    #define fcSupportPNG
    #define fcSupportEXR
    #define fcSupportGIF
    #define fcSupportMP4
    #define fcSupportWebM

    //// MP4-related encoders
    #define fcSupportAAC_FAAC
    //#define fcSupportAAC_Intel
    #define fcSupportH264_OpenH264
    //#define fcSupportH264_NVIDIA
    //#define fcSupportH264_AMD
    //#define fcSupportH264_Intel

    //// WebM-related encoders
    #define fcSupportVPX
    #define fcSupportVorbis
    #define fcSupportOpus

    #define fcSupportWave
    #define fcSupportFlac
#endif

#define fcEnableLogging

#ifdef fcEnableLogging
    void DebugLogImpl(const char* fmt, ...);
    #define fcDebugLog(...) DebugLogImpl(__VA_ARGS__)
#else
    #define fcDebugLog(...)
#endif


class fcContextBase
{
protected:
    virtual ~fcContextBase();
public:
    virtual void release();
    virtual void setOnDeleteCallback(void(*cb)(void*), void *param);

private:
    void(*m_on_delete)(void*) = nullptr;
    void *m_on_delete_param = nullptr;
};

#include "fccore.h"
