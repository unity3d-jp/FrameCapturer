#ifndef fcFoundation_h
#define fcFoundation_h

#include "FrameCapturer.h"

#ifdef fcMSVC
    #define fcThreadLocal __declspec(thread)
#else
    #define fcThreadLocal thread_local
#endif


#ifdef fcWindows
    #include <windows.h>

    typedef HMODULE module_t;
    inline module_t module_load(const char *path) { return ::LoadLibraryA(path); }
    inline void module_close(module_t mod) { ::FreeLibrary(mod); }
    inline void* module_getsymbol(module_t mod, const char *name) { return ::GetProcAddress(mod, name); }

    #define fcModuleExt ".dll"

#else 
    #include <dlfcn.h>

    typedef void* module_t;
    inline module_t module_load(const char *path) { return ::dlopen(path, RTLD_GLOBAL); }
    inline void module_close(module_t mod) { ::dlclose(mod); }
    inline void* module_getsymbol(module_t mod, const char *name) { return ::dlsym(mod, name); }

    #ifdef fcMac
        #define fcModuleExt ".dylib"
    #else
        #define fcModuleExt ".so"
    #endif
#endif


template<int N, class IntType>
inline IntType roundup(IntType v)
{
    return v + (N - v % N);
}

template<class IntType>
inline IntType ceildiv(IntType a, IntType b)
{
    return a / b + (a%b == 0 ? 0 : 1);
}


inline int fcGetPixelSize(fcETextureFormat format)
{
    switch (format)
    {
    case fcE_ARGB32:    return 4;

    case fcE_ARGBHalf:  return 8;
    case fcE_RGHalf:    return 4;
    case fcE_RHalf:     return 2;

    case fcE_ARGBFloat: return 16;
    case fcE_RGFloat:   return 8;
    case fcE_RFloat:    return 4;

    case fcE_ARGBInt:   return 16;
    case fcE_RGInt:     return 8;
    case fcE_RInt:      return 4;
    }
    return 0;
}

#endif // fcFoundation_h
