#ifdef _WIN32
    #define fcWindows
#endif // _WIN32

#define fcSupportEXR
#define fcSupportGIF
#define fcSupportOpenGL
#ifdef fcWindows
    #define fcSupportD3D9
    #define fcSupportD3D11
#endif
#define fcWithTBB



#define fcCLinkage extern "C"
#ifdef fcWindows
    #define fcExport __declspec(dllexport)
    #define fcBreak() DebugBreak()
#else // fcWindows
    #define fcExport
    #define fcBreak() __builtin_trap()
#endif // fcWindows

#ifdef fcDebug
    void fcDebugLogImpl(const char* fmt, ...);
    #define fcDebugLog(...) fcDebugLogImpl(__VA_ARGS__)
    #ifdef fcVerboseDebug
        #define fcDebugLogVerbose(...) fcDebugLogImpl(__VA_ARGS__)
    #else
        #define fcDebugLogVerbose(...)
    #endif
#else
    #define fcDebugLog(...)
    #define fcDebugLogVerbose(...)
#endif



#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

#ifdef fcWithTBB
    #include <tbb/tbb.h>
#endif
