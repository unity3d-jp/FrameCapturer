#ifndef fcMisc_h
#define fcMisc_h

#include "FrameCapturer.h"

#ifdef fcMSVC
    #define fcThreadLocal __declspec(thread)
#else
    #define fcThreadLocal thread_local
#endif

#ifdef fcWindows
    #define fcDLLExt ".dll"
#else 
    #ifdef fcMac
        #define fcDLLExt ".dylib"
    #else
        #define fcDLLExt ".so"
    #endif
#endif

typedef void* module_t;
module_t    DLLLoad(const char *path);
void        DLLUnload(module_t mod);
void*       DLLGetSymbol(module_t mod, const char *name);

void*       AlignedAlloc(size_t size, size_t align);
void        AlignedFree(void *p);

uint64_t    GetCurrentTimeNanosec();

const std::string&  GetPathOfThisModule();


// -------------------------------------------------------------
// Compression
// -------------------------------------------------------------

// if dst.size() == 0, resize to 1024*1024 first.
// while decompress failed because dst.size() is not enough, double it and retry.
bool BZ2Decompress(std::vector<char> &dst, const void *src, size_t src_len);

// return decompressed size if succeeded. 0 if failed.
size_t BZ2DecompressToFile(const char *dst_path, const void *src, size_t src_len);


// -------------------------------------------------------------
// Network
// -------------------------------------------------------------

// abort session if returned false
typedef std::function<bool(const char* data, size_t size)> HTTPCallback;

bool HTTPGet(const std::string &url, std::string &response);
bool HTTPGet(const std::string &url, const HTTPCallback& callback);


// -------------------------------------------------------------
// Math functions
// -------------------------------------------------------------

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

template<class Int>
inline u16 u16_be(Int v_)
{
    u16 v = (u16)v_;
    return ((v >> 8) & 0xFF) | ((v & 0xFF) << 8);
}

template<class Int>
inline u32 u32_be(Int v_)
{
    u32 v = (u32)v_;
    return ((v >> 24) & 0xFF) | (((v >> 16) & 0xFF) << 8) | (((v >> 8) & 0xFF) << 16) | ((v & 0xFF) << 24);
}

template<class Int>
inline u64 u64_be(Int v_)
{
    u64 v = (u64)v_;
    return
        ((v >> 56) & 0xFF) | (((v >> 48) & 0xFF) << 8) | (((v >> 40) & 0xFF) << 16) | (((v >> 32) & 0xFF) << 24) |
        (((v >> 24) & 0xFF) << 32) | (((v >> 16) & 0xFF) << 40) | (((v >> 8) & 0xFF) << 48) | ((v & 0xFF) << 56);
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


int fcGetPixelSize(fcTextureFormat format);
int fcGetPixelSize(fcPixelFormat format);


// F: [](const char *path){} -> void
template<class F>
inline void EachDLLSearchPath(const F& body)
{
    const char** paths = fcGetDLLSearchPaths();
    for (int i = 0; ; ++i) {
        const char *path = paths[i];
        if (path == nullptr) break;
        body(path);
    }
}

#endif // fcMisc_h
