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


typedef uint8_t u8;
typedef half    f16;
typedef float   f32;

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

typedef TRGB<u8>    RGB;
typedef TRGB<f16>   hRGB;
typedef TRGB<f32>   fRGB;
typedef TRGBA<u8>   RGBA;
typedef TRGBA<f16>  hRGBA;
typedef TRGBA<f32>  fRGBA;

template<class T> void CreateVideoData(T *rgba, int width, int height, int frame);
void CreateAudioData(float *samples, int num_samples, int frame);

#endif  // TestCommon_h
