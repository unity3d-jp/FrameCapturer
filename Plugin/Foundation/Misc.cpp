#include "pch.h"
#include "fcFoundation.h"

#ifdef fcWindows
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif


void DebugLogImpl(const char* fmt, ...)
{
#ifdef fcWindows

    char buf[2048];
    va_list vl;
    va_start(vl, fmt);
    vsprintf(buf, fmt, vl);
    ::OutputDebugStringA(buf);
    va_end(vl);

#else // fcWindows

    char buf[2048];
    va_list vl;
    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);

#endif // fcWindows
}



#ifdef fcWindows

module_t DLLLoad(const char *path) { return ::LoadLibraryA(path); }
void DLLUnload(module_t mod) { ::FreeLibrary((HMODULE)mod); }
void* DLLGetSymbol(module_t mod, const char *name) { return ::GetProcAddress((HMODULE)mod, name); }

#else 

module_t DLLLoad(const char *path) { return ::dlopen(path, RTLD_GLOBAL); }
void DLLUnload(module_t mod) { ::dlclose(mod); }
void* DLLGetSymbol(module_t mod, const char *name) { return ::dlsym(mod, name); }

#endif

void DLLAddSearchPath(const char *path_to_add)
{
#ifdef fcWindows
    std::string path;
    path.resize(1024 * 64);

    DWORD ret = ::GetEnvironmentVariableA("PATH", &path[0], (DWORD)path.size());
    path.resize(ret);
    path += ";";
    path += path_to_add;
    ::SetEnvironmentVariableA("PATH", path.c_str());

#else 
    std::string path = getenv("LD_LIBRARY_PATH");
    path += ";";
    path += path_to_add;
    setenv("LD_LIBRARY_PATH", path.c_str(), 1);
#endif
}

const char* DLLGetDirectoryOfCurrentModule()
{
    static std::string s_path;
#ifdef fcWindows
    if (s_path.empty()) {
        char buf[MAX_PATH];
        HMODULE mod = 0;
        ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&DLLGetDirectoryOfCurrentModule, &mod);
        DWORD size = ::GetModuleFileNameA(mod, buf, sizeof(buf));
        for (int i = size - 1; i >= 0; --i) {
            if (buf[i] == '\\') {
                buf[i] = '\0';
                s_path = buf;
                break;
            }
        }
    }
#else 
    // todo:
#endif
    return s_path.c_str();
}



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

double GetCurrentTimeSec()
{
#ifdef fcWindows
    static LARGE_INTEGER g_freq = { 0, 0 };
    if ((u64&)g_freq == 0) {
        ::QueryPerformanceFrequency(&g_freq);
    }

    LARGE_INTEGER t;
    ::QueryPerformanceCounter(&t);
    return double(t.QuadPart) / double(g_freq.QuadPart);
#else
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_nsec / 1000000000.0;
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

fcPixelFormat fcGetPixelFormat(fcTextureFormat format)
{
    switch (format)
    {
    case fcTextureFormat_ARGB32:    return fcPixelFormat_RGBA8;

    case fcTextureFormat_ARGBHalf:  return fcPixelFormat_RGBAHalf;
    case fcTextureFormat_RGHalf:    return fcPixelFormat_RGHalf;
    case fcTextureFormat_RHalf:     return fcPixelFormat_RHalf;

    case fcTextureFormat_ARGBFloat: return fcPixelFormat_RGBAFloat;
    case fcTextureFormat_RGFloat:   return fcPixelFormat_RGFloat;
    case fcTextureFormat_RFloat:    return fcPixelFormat_RFloat;

    case fcTextureFormat_ARGBInt:   return fcPixelFormat_RGBAInt;
    case fcTextureFormat_RGInt:     return fcPixelFormat_RGInt;
    case fcTextureFormat_RInt:      return fcPixelFormat_RInt;
    }
    return fcPixelFormat_Unknown;
}



#ifdef fcUseISPC
    #include "ConvertKernel_ispc.h"
#endif // fcUseISPC

void fcScale(float *data, size_t datasize, float scale)
{
#ifdef fcUseISPC
    ispc::ScaleFloatArray(data, datasize, scale);
#else  // fcUseISPC
    for (size_t i = 0; i < datasize; ++i) {
        data[i] *= scale;
    }
#endif // fcUseISPC
}

void fcConvert(void *dst, size_t dstsize, fcPixelFormat dstfmt, const void *src, size_t srcsize, fcPixelFormat srcfmt)
{
#ifdef fcUseISPC
#else  // fcUseISPC
#endif // fcUseISPC
}

