#include <algorithm>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <tbb/tbb.h>

#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfArray.h>


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


#define fcSupportOpenGL

#define GLEW_STATIC
#include <GL/glew.h>

#ifdef fcWindows
    #define fcSupportD3D11

    #include <windows.h>
    #pragma comment(lib, "Half.lib")
    #pragma comment(lib, "Iex-2_2.lib")
    #pragma comment(lib, "IexMath-2_2.lib")
    #pragma comment(lib, "IlmThread-2_2.lib")
    #pragma comment(lib, "IlmImf-2_2.lib")
    #pragma comment(lib, "zlibstatic.lib")
    #pragma comment(lib, "opengl32.lib")
    #pragma comment(lib, "glew32s.lib")
#endif // fcWindows



#ifdef fcWindows
    #define fcBreak() DebugBreak()
#else // fcWindows
    #define fcBreak() __builtin_trap()
#endif // fcWindows
