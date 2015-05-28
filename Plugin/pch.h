#include <algorithm>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>

#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfArray.h>

#pragma comment(lib, "Half.lib")
#pragma comment(lib, "Iex-2_2.lib")
#pragma comment(lib, "IexMath-2_2.lib")
#pragma comment(lib, "IlmThread-2_2.lib")
#pragma comment(lib, "IlmImf-2_2.lib")
#pragma comment(lib, "zlibstatic.lib")



#ifdef _WIN32
    #define fcWindows
#endif // _WIN32

#define fcCLinkage extern "C"
#ifdef fcWindows
    #define fcExport __declspec(dllexport)
#else // fcWindows
    #define fcExport
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


#ifdef fcWindows
    #include <windows.h>
    #include <d3d11.h>
#define fcSupportD3D11
#endif // fcWindows
//#define fcSupportOpenGL



#ifdef fcWindows
#   define fcBreak() DebugBreak()
#else // fcWindows
#   define fcBreak() __builtin_trap()
#endif // fcWindows
