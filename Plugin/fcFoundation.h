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


inline void* aligned_alloc(size_t size, size_t align)
{
#ifdef _MSC_VER
    return _aligned_malloc(size, align);
#elif defined(__APPLE__)
    return malloc(size);
#else  // _MSC_VER
    return memalign(align, size);
#endif // _MSC_VER
}

inline void aligned_free(void *p)
{
#ifdef _MSC_VER
    _aligned_free(p);
#else  // _MSC_VER
    free(p);
#endif // _MSC_VER
    }


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


inline int fcGetPixelSize(fcTextureFormat format)
{
    switch (format)
    {
    case fcTextureFormat_ARGB32:    return 4;

    case fcTextureFormat_ARGBHalf:  return 8;
    case fcTextureFormat_RGHalf:    return 4;
    case fcTextureFormat_RHalf:     return 2;

    case fcTextureFormat_ARGBFloat: return 16;
    case fcTextureFormat_RGFloat:   return 8;
    case fcTextureFormat_RFloat:    return 4;

    case fcTextureFormat_ARGBInt:   return 16;
    case fcTextureFormat_RGInt:     return 8;
    case fcTextureFormat_RInt:      return 4;
    }
    return 0;
}

inline int fcGetPixelSize(fcPixelFormat format)
{
    switch (format)
    {
    case fcPixelFormat_RGBA8:       return 4;
    case fcPixelFormat_RGB8:        return 3;
    case fcPixelFormat_RG8:         return 2;
    case fcPixelFormat_R8:          return 1;

    case fcPixelFormat_RGBAHalf:    return 8;
    case fcPixelFormat_RGBHalf:     return 6;
    case fcPixelFormat_RGHalf:      return 4;
    case fcPixelFormat_RHalf:       return 2;

    case fcPixelFormat_RGBAFloat:   return 16;
    case fcPixelFormat_RGBFloat:    return 12;
    case fcPixelFormat_RGFloat:     return 8;
    case fcPixelFormat_RFloat:      return 4;

    case fcPixelFormat_RGBAInt:     return 16;
    case fcPixelFormat_RGBInt:      return 12;
    case fcPixelFormat_RGInt:       return 8;
    case fcPixelFormat_RInt:        return 4;
    }
    return 0;
}

#endif // fcFoundation_h
