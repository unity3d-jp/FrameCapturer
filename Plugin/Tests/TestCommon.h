#ifndef TestCommon_h
#define TestCommon_h

#ifdef _MSC_VER
    #pragma warning(disable: 4190)
    #define _CRT_SECURE_NO_WARNINGS
#endif
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <vector>
#include <thread>
#include <half.h>
#include "../FrameCapturer.h"
#include "../Foundation/Buffer.h"
#include "../Foundation/Misc.h"
#include "../Foundation/fcThreadPool.h"


typedef uint8_t u8;
typedef int16_t i16;
typedef half    f16;
typedef float   f32;

template<class T> struct TR {
    T r;

    TR() : r() {}
    TR(T _r) : r(_r) {}
};
template<class T> struct TRG {
    T r, g;

    TRG() : r(), g() {}
    TRG(T _r, T _g) : r(_r), g(_g) {}
};
template<class T> struct TRGB {
    T r, g, b;

    TRGB() : r(), g(), b() {}
    TRGB(T _r, T _g, T _b) : r(_r), g(_g), b(_b) {}
};
template<class T> struct TRGBA {
    T r, g, b, a;

    TRGBA() : r(), g(), b(), a() {}
    TRGBA(T _r, T _g, T _b, T _a) : r(_r), g(_g), b(_b), a(_a) {}
};

typedef TR<u8>      Ru8;
typedef TR<i16>     Ri16;
typedef TR<f16>     Rf16;
typedef TR<f32>     Rf32;

typedef TRG<u8>     RGu8;
typedef TRG<i16>    RGi16;
typedef TRG<f16>    RGf16;
typedef TRG<f32>    RGf32;

typedef TRGB<u8>    RGBu8;
typedef TRGB<i16>   RGBi16;
typedef TRGB<f16>   RGBf16;
typedef TRGB<f32>   RGBf32;

typedef TRGBA<u8>   RGBAu8;
typedef TRGBA<i16>  RGBAi16;
typedef TRGBA<f16>  RGBAf16;
typedef TRGBA<f32>  RGBAf32;

template<class T> struct GetPixelFormat;
#define def(T, E) template<> struct GetPixelFormat<T>  { static const fcPixelFormat value = E; static const char* getName() { return #T; } };
def(Ru8, fcPixelFormat_Ru8)
def(RGu8, fcPixelFormat_RGu8)
def(RGBu8, fcPixelFormat_RGBu8)
def(RGBAu8, fcPixelFormat_RGBAu8)
def(Ri16, fcPixelFormat_Ri16)
def(RGi16, fcPixelFormat_RGi16)
def(RGBi16, fcPixelFormat_RGBi16)
def(RGBAi16, fcPixelFormat_RGBAi16)
def(Rf16, fcPixelFormat_Rf16)
def(RGf16, fcPixelFormat_RGf16)
def(RGBf16, fcPixelFormat_RGBf16)
def(RGBAf16, fcPixelFormat_RGBAf16)
def(Rf32, fcPixelFormat_Rf32)
def(RGf32, fcPixelFormat_RGf32)
def(RGBf32, fcPixelFormat_RGBf32)
def(RGBAf32, fcPixelFormat_RGBAf32)
#undef def

template<class T> void CreateVideoData(T *rgba, int width, int height, int frame);
void CreateAudioData(float *samples, int num_samples, double t, float scale);

#endif  // TestCommon_h
