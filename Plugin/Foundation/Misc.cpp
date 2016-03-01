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
    case fcPixelFormat_RGBAu8:  return 4;
    case fcPixelFormat_RGBu8:   return 3;
    case fcPixelFormat_RGu8:    return 2;
    case fcPixelFormat_Ru8:     return 1;

    case fcPixelFormat_RGBAf16:
    case fcPixelFormat_RGBAi16: return 8;
    case fcPixelFormat_RGBf16:
    case fcPixelFormat_RGBi16:  return 6;
    case fcPixelFormat_RGf16:
    case fcPixelFormat_RGi16:   return 4;
    case fcPixelFormat_Rf16:
    case fcPixelFormat_Ri16:    return 2;

    case fcPixelFormat_RGBAf32: 
    case fcPixelFormat_RGBAi32: return 16;
    case fcPixelFormat_RGBf32:
    case fcPixelFormat_RGBi32:  return 12;
    case fcPixelFormat_RGf32:
    case fcPixelFormat_RGi32:   return 8;
    case fcPixelFormat_Rf32:
    case fcPixelFormat_Ri32:    return 4;
    }
    return 0;
}

fcPixelFormat fcGetPixelFormat(fcTextureFormat format)
{
    switch (format)
    {
    case fcTextureFormat_ARGB32:    return fcPixelFormat_RGBAu8;

    case fcTextureFormat_ARGBHalf:  return fcPixelFormat_RGBAf16;
    case fcTextureFormat_RGHalf:    return fcPixelFormat_RGf16;
    case fcTextureFormat_RHalf:     return fcPixelFormat_Rf16;

    case fcTextureFormat_ARGBFloat: return fcPixelFormat_RGBAf32;
    case fcTextureFormat_RGFloat:   return fcPixelFormat_RGf32;
    case fcTextureFormat_RFloat:    return fcPixelFormat_Rf32;

    case fcTextureFormat_ARGBInt:   return fcPixelFormat_RGBAi32;
    case fcTextureFormat_RGInt:     return fcPixelFormat_RGi32;
    case fcTextureFormat_RInt:      return fcPixelFormat_Ri32;
    }
    return fcPixelFormat_Unknown;
}


#define fcEnableISPCConverter
#define fcEnableCppConverter


#ifdef fcEnableISPCConverter
    #include "ConvertKernel_ispc.h"
#endif // fcEnableISPCConverter


void fcScale(float *data, size_t size, float scale)
{
#ifdef fcEnableISPCConverter
    ispc::ScaleFloatArray(data, (uint32_t)size, scale);
#else  // fcEnableISPCConverter
    for (size_t i = 0; i < datasize; ++i) {
        data[i] *= scale;
    }
#endif // fcEnableISPCConverter
}



#ifdef fcEnableCppConverter
template<class T> struct tvec1;
template<class T> struct tvec2;
template<class T> struct tvec3;
template<class T> struct tvec4;

template<class T> struct tvec1
{
    T x;
    tvec1() {} // !not clear members!
    tvec1(T a) : x(a) {}
    operator T&() { return x; }
    template<class U> tvec1(const tvec1<U>& src);
    template<class U> tvec1(const tvec2<U>& src);
    template<class U> tvec1(const tvec3<U>& src);
    template<class U> tvec1(const tvec4<U>& src);
};

template<class T> struct tvec2
{
    T x, y;
    tvec2() {} // !not clear members!
    tvec2(T a) : x(a), y(a) {}
    tvec2(T a, T b) : x(a), y(b) {}
    template<class U> tvec2(const tvec1<U>& src);
    template<class U> tvec2(const tvec2<U>& src);
    template<class U> tvec2(const tvec3<U>& src);
    template<class U> tvec2(const tvec4<U>& src);
};

template<class T> struct tvec3
{
    T x, y, z;
    tvec3() {} // !not clear members!
    tvec3(T a) : x(a), y(a), z(a) {}
    tvec3(T a, T b, T c) : x(a), y(b), z(c) {}
    template<class U> tvec3(const tvec1<U>& src);
    template<class U> tvec3(const tvec2<U>& src);
    template<class U> tvec3(const tvec3<U>& src);
    template<class U> tvec3(const tvec4<U>& src);
};

template<class T> struct tvec4
{
    T x, y, z, w;
    tvec4() {} // !not clear members!
    tvec4(T a) : x(a), y(a), z(a), w(a) {}
    tvec4(T a, T b, T c, T d) : x(a), y(b), z(c), w(d) {}
    template<class U> tvec4(const tvec1<U>& src);
    template<class U> tvec4(const tvec2<U>& src);
    template<class U> tvec4(const tvec3<U>& src);
    template<class U> tvec4(const tvec4<U>& src);
};

template<class DstType, class SrcType> inline DstType tScalar(SrcType src) { return DstType(src); }

template<class T> template<class U> tvec1<T>::tvec1(const tvec1<U>& src) : x(tScalar<T>(src.x)) {}
template<class T> template<class U> tvec1<T>::tvec1(const tvec2<U>& src) : x(tScalar<T>(src.x)) {}
template<class T> template<class U> tvec1<T>::tvec1(const tvec3<U>& src) : x(tScalar<T>(src.x)) {}
template<class T> template<class U> tvec1<T>::tvec1(const tvec4<U>& src) : x(tScalar<T>(src.x)) {}

template<class T> template<class U> tvec2<T>::tvec2(const tvec1<U>& src) : x(tScalar<T>(src.x)), y() {}
template<class T> template<class U> tvec2<T>::tvec2(const tvec2<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)) {}
template<class T> template<class U> tvec2<T>::tvec2(const tvec3<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)) {}
template<class T> template<class U> tvec2<T>::tvec2(const tvec4<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)) {}

template<class T> template<class U> tvec3<T>::tvec3(const tvec1<U>& src) : x(tScalar<T>(src.x)), y(), z() {}
template<class T> template<class U> tvec3<T>::tvec3(const tvec2<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)), z() {}
template<class T> template<class U> tvec3<T>::tvec3(const tvec3<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)), z(tScalar<T>(src.z)) {}
template<class T> template<class U> tvec3<T>::tvec3(const tvec4<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)), z(tScalar<T>(src.z)) {}

template<class T> template<class U> tvec4<T>::tvec4(const tvec1<U>& src) : x(tScalar<T>(src.x)), y(), z(), w() {}
template<class T> template<class U> tvec4<T>::tvec4(const tvec2<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)), z(), w() {}
template<class T> template<class U> tvec4<T>::tvec4(const tvec3<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)), z(tScalar<T>(src.z)), w() {}
template<class T> template<class U> tvec4<T>::tvec4(const tvec4<U>& src) : x(tScalar<T>(src.x)), y(tScalar<T>(src.y)), z(tScalar<T>(src.z)), w(tScalar<T>(src.w)) {}

template<class T> tvec3<T> operator+(const tvec3<T> &a, const tvec3<T> &b) { return tvec3<T>(a.x + b.x, a.y + b.y, a.z + b.z); }
template<class T> tvec3<T> operator-(const tvec3<T> &a, const tvec3<T> &b) { return tvec3<T>(a.x - b.x, a.y - b.y, a.z - b.z); }
template<class T> tvec3<T> operator*(const tvec3<T> &a, const tvec3<T> &b) { return tvec3<T>(a.x * b.x, a.y * b.y, a.z * b.z); }
template<class T> tvec3<T> operator/(const tvec3<T> &a, const tvec3<T> &b) { return tvec3<T>(a.x / b.x, a.y / b.y, a.z / b.z); }
template<class T> tvec3<T> operator*(const tvec3<T> &a, T b) { return tvec3<T>(a.x * b, a.y * b, a.z * b); }
template<class T> tvec3<T> operator/(const tvec3<T> &a, T b) { return tvec3<T>(a.x / b, a.y / b, a.z / b); }

#include <half.h>
#pragma comment(lib, "Half.lib")

typedef tvec1<uint8_t> byte1;
typedef tvec2<uint8_t> byte2;
typedef tvec3<uint8_t> byte3;
typedef tvec4<uint8_t> byte4;
typedef tvec1<int16_t> short1;
typedef tvec2<int16_t> short2;
typedef tvec3<int16_t> short3;
typedef tvec4<int16_t> short4;
typedef tvec1<int32_t> int1;
typedef tvec2<int32_t> int2;
typedef tvec3<int32_t> int3;
typedef tvec4<int32_t> int4;
typedef tvec1<int64_t> long1;
typedef tvec2<int64_t> long2;
typedef tvec3<int64_t> long3;
typedef tvec4<int64_t> long4;
typedef tvec1<half>    half1;
typedef tvec2<half>    half2;
typedef tvec3<half>    half3;
typedef tvec4<half>    half4;
typedef tvec1<float>   float1;
typedef tvec2<float>   float2;
typedef tvec3<float>   float3;
typedef tvec4<float>   float4;

template<int I> struct tGetPixelFormatType;
#define def(I, T) template<> struct tGetPixelFormatType<I>  { typedef T type; };
def(fcPixelFormat_Ru8    , byte1  )
def(fcPixelFormat_RGu8   , byte2  )
def(fcPixelFormat_RGBu8  , byte3  )
def(fcPixelFormat_RGBAu8 , byte4  )
def(fcPixelFormat_Ri16   , short1 )
def(fcPixelFormat_RGi16  , short2 )
def(fcPixelFormat_RGBi16 , short3 )
def(fcPixelFormat_RGBAi16, short4 )
def(fcPixelFormat_Rf16   , half1  )
def(fcPixelFormat_RGf16  , half2  )
def(fcPixelFormat_RGBf16 , half3  )
def(fcPixelFormat_RGBAf16, half4  )
def(fcPixelFormat_Rf32   , float1 )
def(fcPixelFormat_RGf32  , float2 )
def(fcPixelFormat_RGBf32 , float3 )
def(fcPixelFormat_RGBAf32, float4 )
#undef def

template<> inline half tScalar(int32_t src) { return half((float)src); }
template<> inline half tScalar(int64_t src) { return half((float)src); }


template<class DstType, class SrcType>
struct tConvertArrayImpl
{
    void operator()(void *&dst_, const void *src_, size_t num_elements)
    {
        DstType *dst = (DstType*)dst_;
        const SrcType *src = (const SrcType*)src_;
        for (size_t i = 0; i < num_elements; ++i) {
            dst[i] = DstType(src[i]);
        }
    }
};
template<class T>
struct tConvertArrayImpl<T, T>
{
    void operator()(void *&dst, const void *src, size_t num_elements)
    {
        dst = (void*)src;
    }
};

template<class DstType, class SrcType> inline void tConvertArray(void *&dst, const void *src, size_t size)
{
    tConvertArrayImpl<DstType, SrcType>()(dst, src, size);
}
const void* fcConvert_Cpp(void *dst, fcPixelFormat dst_fmt, const void *src, fcPixelFormat src_fmt, size_t size)
{
#define TCase(Dst, Src)\
    case Src: tConvertArray<tGetPixelFormatType<Dst>::type, tGetPixelFormatType<Src>::type>(dst, src, size);

#define TBlock(Dst)\
    case Dst:\
        switch (src_fmt) {\
        TCase(Dst, fcPixelFormat_Ru8    )\
        TCase(Dst, fcPixelFormat_RGu8   )\
        TCase(Dst, fcPixelFormat_RGBu8  )\
        TCase(Dst, fcPixelFormat_RGBAu8 )\
        TCase(Dst, fcPixelFormat_Ri16   )\
        TCase(Dst, fcPixelFormat_RGi16  )\
        TCase(Dst, fcPixelFormat_RGBi16 )\
        TCase(Dst, fcPixelFormat_RGBAi16)\
        TCase(Dst, fcPixelFormat_Rf16   )\
        TCase(Dst, fcPixelFormat_RGf16  )\
        TCase(Dst, fcPixelFormat_RGBf16 )\
        TCase(Dst, fcPixelFormat_RGBAf16)\
        TCase(Dst, fcPixelFormat_Rf32   )\
        TCase(Dst, fcPixelFormat_RGf32  )\
        TCase(Dst, fcPixelFormat_RGBf32 )\
        TCase(Dst, fcPixelFormat_RGBAf32)\
        } break;

    switch (dst_fmt) {
        TBlock(fcPixelFormat_Ru8    )
        TBlock(fcPixelFormat_RGu8   )
        TBlock(fcPixelFormat_RGBu8  )
        TBlock(fcPixelFormat_RGBAu8 )
        TBlock(fcPixelFormat_Ri16   )
        TBlock(fcPixelFormat_RGi16  )
        TBlock(fcPixelFormat_RGBi16 )
        TBlock(fcPixelFormat_RGBAi16)
        TBlock(fcPixelFormat_Rf16   )
        TBlock(fcPixelFormat_RGf16  )
        TBlock(fcPixelFormat_RGBf16 )
        TBlock(fcPixelFormat_RGBAf16)
        TBlock(fcPixelFormat_Rf32   )
        TBlock(fcPixelFormat_RGf32  )
        TBlock(fcPixelFormat_RGBf32 )
        TBlock(fcPixelFormat_RGBAf32)
    }

#undef TBlock
#undef TCase
    return dst;
}
#endif // fcEnableCppConverter

#ifdef fcEnableISPCConverter
const void* fcConvert_ISPC(void *dst, fcPixelFormat dstfmt, const void *src, fcPixelFormat srcfmt, size_t size_)
{
    uint32_t size = (uint32_t)size_;
    switch (srcfmt) {
    case fcPixelFormat_RGBAu8:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: return src;
        case fcPixelFormat_RGBu8: ispc::RGBAu8ToRGBu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGu8: ispc::RGBAu8ToRGu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Ru8: ispc::RGBAu8ToRu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::RGBAu8ToRGBAf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::RGBAu8ToRGBf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGf16: ispc::RGBAu8ToRGf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Rf16: ispc::RGBAu8ToRf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::RGBAu8ToRGBAf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::RGBAu8ToRGBf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGf32: ispc::RGBAu8ToRGf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Rf32: ispc::RGBAu8ToRf32((float*)dst, (uint8_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RGBu8:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::RGBu8ToRGBAu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBu8: return src;
        case fcPixelFormat_RGu8: ispc::RGBu8ToRGu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Ru8: ispc::RGBu8ToRu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::RGBu8ToRGBAf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::RGBu8ToRGBf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGf16: ispc::RGBu8ToRGf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Rf16: ispc::RGBu8ToRf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::RGBu8ToRGBAf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::RGBu8ToRGBf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGf32: ispc::RGBu8ToRGf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Rf32: ispc::RGBu8ToRf32((float*)dst, (uint8_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RGu8:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::RGu8ToRGBAu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::RGu8ToRGBu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGu8: return src;
        case fcPixelFormat_Ru8: ispc::RGu8ToRu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::RGu8ToRGBAf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::RGu8ToRGBf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGf16: ispc::RGu8ToRGf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Rf16: ispc::RGu8ToRf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::RGu8ToRGBAf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::RGu8ToRGBf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGf32: ispc::RGu8ToRGf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Rf32: ispc::RGu8ToRf32((float*)dst, (uint8_t*)src, size); break;
        }
        break;
    case fcPixelFormat_Ru8:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::Ru8ToRGBAu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::Ru8ToRGBu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGu8: ispc::Ru8ToRGu8((uint8_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Ru8: return src;
        case fcPixelFormat_RGBAf16: ispc::Ru8ToRGBAf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::Ru8ToRGBf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGf16: ispc::Ru8ToRGf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Rf16: ispc::Ru8ToRf16((int16_t*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::Ru8ToRGBAf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::Ru8ToRGBf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_RGf32: ispc::Ru8ToRGf32((float*)dst, (uint8_t*)src, size); break;
        case fcPixelFormat_Rf32: ispc::Ru8ToRf32((float*)dst, (uint8_t*)src, size); break;
        }
        break;

    case fcPixelFormat_RGBAf16:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::RGBAf16ToRGBAu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::RGBAf16ToRGBu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGu8: ispc::RGBAf16ToRGu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Ru8: ispc::RGBAf16ToRu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAi16: ispc::RGBAf16ToRGBAi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBi16: ispc::RGBAf16ToRGBi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGi16: ispc::RGBAf16ToRGi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Ri16: ispc::RGBAf16ToRi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAf16: return src;
        case fcPixelFormat_RGBf16: ispc::RGBAf16ToRGBf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGf16: ispc::RGBAf16ToRGf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Rf16: ispc::RGBAf16ToRf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::RGBAf16ToRGBAf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::RGBAf16ToRGBf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGf32: ispc::RGBAf16ToRGf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Rf32: ispc::RGBAf16ToRf32((float*)dst, (int16_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RGBf16:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::RGBf16ToRGBAu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::RGBf16ToRGBu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGu8: ispc::RGBf16ToRGu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Ru8: ispc::RGBf16ToRu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAi16: ispc::RGBf16ToRGBAi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBi16: ispc::RGBf16ToRGBi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGi16: ispc::RGBf16ToRGi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Ri16: ispc::RGBf16ToRi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::RGBf16ToRGBAf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBf16: return src;
        case fcPixelFormat_RGf16: ispc::RGBf16ToRGf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Rf16: ispc::RGBf16ToRf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::RGBf16ToRGBAf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::RGBf16ToRGBf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGf32: ispc::RGBf16ToRGf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Rf32: ispc::RGBf16ToRf32((float*)dst, (int16_t*)src, size); break;
        }
        break;
    case fcPixelFormat_RGf16:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::RGf16ToRGBAu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::RGf16ToRGBu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGu8: ispc::RGf16ToRGu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Ru8: ispc::RGf16ToRu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAi16: ispc::RGf16ToRGBAi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBi16: ispc::RGf16ToRGBi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGi16: ispc::RGf16ToRGi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Ri16: ispc::RGf16ToRi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::RGf16ToRGBAf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::RGf16ToRGBf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGf16: return src;
        case fcPixelFormat_Rf16: ispc::RGf16ToRf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::RGf16ToRGBAf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::RGf16ToRGBf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGf32: ispc::RGf16ToRGf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Rf32: ispc::RGf16ToRf32((float*)dst, (int16_t*)src, size); break;
        }
        break;
    case fcPixelFormat_Rf16:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::Rf16ToRGBAu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::Rf16ToRGBu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGu8: ispc::Rf16ToRGu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Ru8: ispc::Rf16ToRu8((uint8_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAi16: ispc::Rf16ToRGBAi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBi16: ispc::Rf16ToRGBi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGi16: ispc::Rf16ToRGi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Ri16: ispc::Rf16ToRi16((uint16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::Rf16ToRGBAf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::Rf16ToRGBf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGf16: ispc::Rf16ToRGf16((int16_t*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Rf16: return src;
        case fcPixelFormat_RGBAf32: ispc::Rf16ToRGBAf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::Rf16ToRGBf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_RGf32: ispc::Rf16ToRGf32((float*)dst, (int16_t*)src, size); break;
        case fcPixelFormat_Rf32: ispc::Rf16ToRf32((float*)dst, (int16_t*)src, size); break;
        }
        break;

    case fcPixelFormat_RGBAf32:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::RGBAf32ToRGBAu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::RGBAf32ToRGBu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGu8: ispc::RGBAf32ToRGu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Ru8: ispc::RGBAf32ToRu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAi16: ispc::RGBAf32ToRGBAi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBi16: ispc::RGBAf32ToRGBi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGi16: ispc::RGBAf32ToRGi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Ri16: ispc::RGBAf32ToRi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::RGBAf32ToRGBAf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::RGBAf32ToRGBf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGf16: ispc::RGBAf32ToRGf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Rf16: ispc::RGBAf32ToRf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAf32: return src;
        case fcPixelFormat_RGBf32: ispc::RGBAf32ToRGBf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGf32: ispc::RGBAf32ToRGf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_Rf32: ispc::RGBAf32ToRf32((float*)dst, (float*)src, size); break;
        }
        break;
    case fcPixelFormat_RGBf32:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::RGBf32ToRGBAu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::RGBf32ToRGBu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGu8: ispc::RGBf32ToRGu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Ru8: ispc::RGBf32ToRu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAi16: ispc::RGBf32ToRGBAi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBi16: ispc::RGBf32ToRGBi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGi16: ispc::RGBf32ToRGi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Ri16: ispc::RGBf32ToRi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::RGBf32ToRGBAf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::RGBf32ToRGBf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGf16: ispc::RGBf32ToRGf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Rf16: ispc::RGBf32ToRf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::RGBf32ToRGBAf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBf32: return src;
        case fcPixelFormat_RGf32: ispc::RGBf32ToRGf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_Rf32: ispc::RGBf32ToRf32((float*)dst, (float*)src, size); break;
        }
        break;
    case fcPixelFormat_RGf32:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::RGf32ToRGBAu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::RGf32ToRGBu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGu8: ispc::RGf32ToRGu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Ru8: ispc::RGf32ToRu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAi16: ispc::RGf32ToRGBAi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBi16: ispc::RGf32ToRGBi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGi16: ispc::RGf32ToRGi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Ri16: ispc::RGf32ToRi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::RGf32ToRGBAf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::RGf32ToRGBf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGf16: ispc::RGf32ToRGf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Rf16: ispc::RGf32ToRf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::RGf32ToRGBAf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::RGf32ToRGBf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGf32: return src;
        case fcPixelFormat_Rf32: ispc::RGf32ToRf32((float*)dst, (float*)src, size); break;
        }
        break;
    case fcPixelFormat_Rf32:
        switch (dstfmt) {
        case fcPixelFormat_RGBAu8: ispc::Rf32ToRGBAu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBu8: ispc::Rf32ToRGBu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGu8: ispc::Rf32ToRGu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Ru8: ispc::Rf32ToRu8((uint8_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAi16: ispc::Rf32ToRGBAi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBi16: ispc::Rf32ToRGBi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGi16: ispc::Rf32ToRGi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Ri16: ispc::Rf32ToRi16((uint16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAf16: ispc::Rf32ToRGBAf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBf16: ispc::Rf32ToRGBf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGf16: ispc::Rf32ToRGf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_Rf16: ispc::Rf32ToRf16((int16_t*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBAf32: ispc::Rf32ToRGBAf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGBf32: ispc::Rf32ToRGBf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_RGf32: ispc::Rf32ToRGf32((float*)dst, (float*)src, size); break;
        case fcPixelFormat_Rf32: return src;
        }
        break;
    }
    return dst;
}
#endif // fcEnableISPCConverter



const void* fcConvert(void *dst, fcPixelFormat dstfmt, const void *src, fcPixelFormat srcfmt, size_t size)
{
#ifdef fcEnableISPCConverter
    return fcConvert_ISPC(dst, dstfmt, src, srcfmt, size);
#elif fcEnableCppConverter
    return fcConvert_Cpp(dst, dstfmt, src, srcfmt, size);
#endif
}

