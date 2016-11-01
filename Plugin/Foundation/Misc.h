#pragma once

#include "Buffer.h"

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
void        DLLAddSearchPath(const char *path);
const char* DLLGetDirectoryOfCurrentModule();

void*       AlignedAlloc(size_t size, size_t align);
void        AlignedFree(void *p);

double      GetCurrentTimeInSeconds();

// execute command and **wait until it ends**
// return exit-code
int         Execute(const char *command);


// -------------------------------------------------------------
// Compression
// -------------------------------------------------------------

// src is on-memory bz2 data
// while decompress failed because of dst.size() is not enough, double it and retry.
// (if dst.size() == 0, resize to 1024*1024 before first try)
// dst.size() will be size of decompressed buffer size if succeeded.
bool    BZ2Decompress(Buffer &dst, const void *src, size_t src_len);

// src is on-memory bz2 data
// return decompressed size if successfully decompressed and written to file. otherwise 0.
size_t  BZ2DecompressToFile(const char *dst_path, const void *src, size_t src_len);

// return decompressed file count
typedef std::function<void(const char*)> UnzipFileHandler;
size_t Unzip(const char *dst_path, const char *archive, const UnzipFileHandler& h = UnzipFileHandler());


// -------------------------------------------------------------
// Network
// -------------------------------------------------------------

// abort session if returned false
typedef std::function<bool(const char* data, size_t size)> HTTPCallback;

// return true if successfully get content and HTTP status code is 200 (OK). otherwise false.
bool HTTPGet(const std::string &url, std::string &response, int *status_code = nullptr);
bool HTTPGet(const std::string &url, const HTTPCallback& callback, int *status_code = nullptr);


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

// i.e: 
//  ceildiv(31, 16) : 2
//  ceildiv(32, 16) : 2
//  ceildiv(33, 16) : 3
template<class IntType>
inline IntType ceildiv(IntType a, IntType b)
{
    return a / b + (a%b == 0 ? 0 : 1);
}

// i.e:
//  roundup<16>(31) : 32
//  roundup<16>(32) : 32
//  roundup<16>(33) : 48
template<int N, class IntType>
inline IntType roundup(IntType v)
{
    return ceildiv(v, N) * N;
}
