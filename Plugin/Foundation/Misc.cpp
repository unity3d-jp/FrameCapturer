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

void fcScale(float *data, size_t size, float scale)
{
#ifdef fcUseISPC
    ispc::ScaleFloatArray(data, (uint32_t)size, scale);
#else  // fcUseISPC
    for (size_t i = 0; i < datasize; ++i) {
        data[i] *= scale;
    }
#endif // fcUseISPC
}

void fcConvert(void *dst, fcPixelFormat dstfmt, const void *src, fcPixelFormat srcfmt, size_t size_)
{
    uint32_t size = (uint32_t)size_;
#ifdef fcUseISPC
    switch (srcfmt) {
    case fcPixelFormat_RGBA8:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: break;
        case fcPixelFormat_RGB8: ispc::RGBAu8ToRGBu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RG8: ispc::RGBAu8ToRGu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_R8: ispc::RGBAu8ToRu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::RGBAu8ToRGBAf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::RGBAu8ToRGBf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::RGBAu8ToRGf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RHalf: ispc::RGBAu8ToRf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::RGBAu8ToRGBAf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::RGBAu8ToRGBf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::RGBAu8ToRGf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RFloat: ispc::RGBAu8ToRf32((float*)dst, (uint8_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RGB8:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::RGBu8ToRGBAu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGB8: break;
        case fcPixelFormat_RG8: ispc::RGBu8ToRGu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_R8: ispc::RGBu8ToRu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::RGBu8ToRGBAf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::RGBu8ToRGBf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::RGBu8ToRGf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RHalf: ispc::RGBu8ToRf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::RGBu8ToRGBAf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::RGBu8ToRGBf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::RGBu8ToRGf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RFloat: ispc::RGBu8ToRf32((float*)dst, (uint8_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RG8:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::RGu8ToRGBAu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGB8: ispc::RGu8ToRGBu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RG8: break;
        case fcPixelFormat_R8: ispc::RGu8ToRu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::RGu8ToRGBAf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::RGu8ToRGBf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::RGu8ToRGf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RHalf: ispc::RGu8ToRf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::RGu8ToRGBAf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::RGu8ToRGBf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::RGu8ToRGf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RFloat: ispc::RGu8ToRf32((float*)dst, (uint8_t*)src, size); break;
        }
        break;
    case fcPixelFormat_R8:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::Ru8ToRGBAu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGB8: ispc::Ru8ToRGBu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RG8: ispc::Ru8ToRGu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_R8: break;
        case fcPixelFormat_RGBAHalf: ispc::Ru8ToRGBAf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::Ru8ToRGBf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::Ru8ToRGf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RHalf: ispc::Ru8ToRf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::Ru8ToRGBAf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::Ru8ToRGBf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::Ru8ToRGf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RFloat: ispc::Ru8ToRf32((float*)dst, (uint8_t*)src, size); break;
        }
        break;

    case fcPixelFormat_RGBAHalf:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::RGBAf16ToRGBAu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGB8: ispc::RGBAf16ToRGBu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RG8: ispc::RGBAf16ToRGu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_R8: ispc::RGBAf16ToRu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAHalf: break;
        case fcPixelFormat_RGBHalf: ispc::RGBAf16ToRGBf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::RGBAf16ToRGf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RHalf: ispc::RGBAf16ToRf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::RGBAf16ToRGBAf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::RGBAf16ToRGBf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::RGBAf16ToRGf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RFloat: ispc::RGBAf16ToRf32((float*)dst, (int16_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RGBHalf:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::RGBf16ToRGBAu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGB8: ispc::RGBf16ToRGBu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RG8: ispc::RGBf16ToRGu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_R8: ispc::RGBf16ToRu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::RGBf16ToRGBAf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBHalf: break;
        case fcPixelFormat_RGHalf: ispc::RGBf16ToRGf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RHalf: ispc::RGBf16ToRf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::RGBf16ToRGBAf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::RGBf16ToRGBf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::RGBf16ToRGf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RFloat: ispc::RGBf16ToRf32((float*)dst, (int16_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RGHalf:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::RGf16ToRGBAu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGB8: ispc::RGf16ToRGBu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RG8: ispc::RGf16ToRGu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_R8: ispc::RGf16ToRu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::RGf16ToRGBAf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::RGf16ToRGBf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGHalf: break;
        case fcPixelFormat_RHalf: ispc::RGf16ToRf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::RGf16ToRGBAf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::RGf16ToRGBf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::RGf16ToRGf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RFloat: ispc::RGf16ToRf32((float*)dst, (int16_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RHalf:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::Rf16ToRGBAu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGB8: ispc::Rf16ToRGBu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RG8: ispc::Rf16ToRGu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_R8: ispc::Rf16ToRu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::Rf16ToRGBAf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::Rf16ToRGBf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::Rf16ToRGf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RHalf: break;
        case fcPixelFormat_RGBAFloat: ispc::Rf16ToRGBAf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::Rf16ToRGBf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::Rf16ToRGf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RFloat: ispc::Rf16ToRf32((float*)dst, (int16_t*)src, size); break;
        }
        break;

    case fcPixelFormat_RGBAFloat:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::RGBAf32ToRGBAu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGB8: ispc::RGBAf32ToRGBu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RG8: ispc::RGBAf32ToRGu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_R8: ispc::RGBAf32ToRu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::RGBAf32ToRGBAf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::RGBAf32ToRGBf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::RGBAf32ToRGf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RHalf: ispc::RGBAf32ToRf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAFloat: break;
        case fcPixelFormat_RGBFloat: ispc::RGBAf32ToRGBf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::RGBAf32ToRGf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RFloat: ispc::RGBAf32ToRf32((float*)dst, (float*)src, size); break;
        }
        break;
    case fcPixelFormat_RGBFloat:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::RGBf32ToRGBAu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGB8: ispc::RGBf32ToRGBu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RG8: ispc::RGBf32ToRGu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_R8: ispc::RGBf32ToRu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::RGBf32ToRGBAf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::RGBf32ToRGBf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::RGBf32ToRGf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RHalf: ispc::RGBf32ToRf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::RGBf32ToRGBAf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBFloat: break;
        case fcPixelFormat_RGFloat: ispc::RGBf32ToRGf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RFloat: ispc::RGBf32ToRf32((float*)dst, (float*)src, size); break;
        }
        break;
    case fcPixelFormat_RGFloat:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::RGf32ToRGBAu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGB8: ispc::RGf32ToRGBu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RG8: ispc::RGf32ToRGu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_R8: ispc::RGf32ToRu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::RGf32ToRGBAf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::RGf32ToRGBf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::RGf32ToRGf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RHalf: ispc::RGf32ToRf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::RGf32ToRGBAf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::RGf32ToRGBf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGFloat: break;
        case fcPixelFormat_RFloat: ispc::RGf32ToRf32((float*)dst, (float*)src, size); break;
        }
        break;
    case fcPixelFormat_RFloat:
        switch (dstfmt) {
        case fcPixelFormat_RGBA8: ispc::Rf32ToRGBAu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGB8: ispc::Rf32ToRGBu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RG8: ispc::Rf32ToRGu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_R8: ispc::Rf32ToRu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAHalf: ispc::Rf32ToRGBAf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBHalf: ispc::Rf32ToRGBf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGHalf: ispc::Rf32ToRGf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RHalf: ispc::Rf32ToRf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAFloat: ispc::Rf32ToRGBAf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBFloat: ispc::Rf32ToRGBf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGFloat: ispc::Rf32ToRGf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RFloat: break;
        }
        break;

    }
#else  // fcUseISPC
#endif // fcUseISPC
}

