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

#define fcImpl

#ifdef _WIN32
    #define fcBreak() DebugBreak()
#else
    #define fcBreak() __builtin_trap()
#endif

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

#ifdef _WIN32
    #define fcSupportOpenGL
    #define fcSupportD3D9
    #define fcSupportD3D11

    #define fcSupportOpenH264
    #define fcSupportNVH264
    #define fcSupportAMDH264
#else
    #define fcSupportOpenGL

    #define fcSupportOpenH264
    #define fcSupportNVH264
#endif
