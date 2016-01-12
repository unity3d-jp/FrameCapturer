#include "pch.h"
#include "FrameCapturer.h"
#include "Misc.h"

#ifdef fcWindows
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#ifdef fcWindows

module_t DLLLoad(const char *path) { return ::LoadLibraryA(path); }
void DLLUnload(module_t mod) { ::FreeLibrary((HMODULE)mod); }
void* DLLGetSymbol(module_t mod, const char *name) { return ::GetProcAddress((HMODULE)mod, name); }

#else 

module_t DLLLoad(const char *path) { return ::dlopen(path, RTLD_GLOBAL); }
void DLLUnload(module_t mod) { ::dlclose(mod); }
void* DLLGetSymbol(module_t mod, const char *name) { return ::dlsym(mod, name); }

#endif


void* AlignedAlloc(size_t size, size_t align)
{
#ifdef fcWindows
    return _aligned_malloc(size, align);
#elif defined(fcMac)
    return malloc(size);
#else
    return memalign(align, size);
#endif
}

void AlignedFree(void *p)
{
#ifdef fcWindows
    _aligned_free(p);
#else
    free(p);
#endif
}

uint64_t GetCurrentTimeNanosec()
{
#ifdef fcWindows
    static LARGE_INTEGER g_freq = { 0, 0 };
    if ((u64&)g_freq == 0) {
        ::QueryPerformanceFrequency(&g_freq);
    }

    LARGE_INTEGER t;
    ::QueryPerformanceCounter(&t);
    return u64(double(t.QuadPart) / double(g_freq.QuadPart) * 1000000000.0);
#else
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_nsec;
#endif
}


const std::string& GetPathOfThisModule()
{
#ifdef fcWindows
    static std::string s_path;;
    if (s_path.empty()) {
        char buf[MAX_PATH];
        HMODULE mod = 0;
        ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&GetPathOfThisModule, &mod);
        DWORD size = ::GetModuleFileNameA(mod, buf, sizeof(buf));
        for (int i = size - 1; i >= 0; --i) {
            if (buf[i] == '\\') {
                buf[i] = '\0';
                s_path = buf;
                break;
            }
        }
    }
    return s_path;
#else 
    // todo:
    static std::string s_path;
    return s_path;
#endif
}


int fcGetPixelSize(fcTextureFormat format)
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

int fcGetPixelSize(fcPixelFormat format)
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
