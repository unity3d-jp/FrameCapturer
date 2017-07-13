#pragma once

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
#include <OpenEXR/half.h>
#include "../fccore/fccore.h"
#include "../fccore/Foundation/fcFoundation.h"


using u8  = uint8_t;
using i16 = int16_t;
using f16 = half;
using f32 = float;

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

using Ru8     = TR<u8>;
using Ri16    = TR<i16>;
using Rf16    = TR<f16>;
using Rf32    = TR<f32>;

using RGu8    = TRG<u8>;
using RGi16   = TRG<i16>;
using RGf16   = TRG<f16>;
using RGf32   = TRG<f32>;

using RGBu8   = TRGB<u8>;
using RGBi16  = TRGB<i16>;
using RGBf16  = TRGB<f16>;
using RGBf32  = TRGB<f32>;
using RGBAu8  = TRGBA<u8>;

using RGBAi16 = TRGBA<i16>;
using RGBAf16 = TRGBA<f16>;
using RGBAf32 = TRGBA<f32>;


template<class T> struct GetPixelFormat;
#define Def(T, E) template<> struct GetPixelFormat<T>  { static const fcPixelFormat value = E; static const char* getName() { return #T; } };
Def(Ru8, fcPixelFormat_Ru8)
Def(RGu8, fcPixelFormat_RGu8)
Def(RGBu8, fcPixelFormat_RGBu8)
Def(RGBAu8, fcPixelFormat_RGBAu8)
Def(Ri16, fcPixelFormat_Ri16)
Def(RGi16, fcPixelFormat_RGi16)
Def(RGBi16, fcPixelFormat_RGBi16)
Def(RGBAi16, fcPixelFormat_RGBAi16)
Def(Rf16, fcPixelFormat_Rf16)
Def(RGf16, fcPixelFormat_RGf16)
Def(RGBf16, fcPixelFormat_RGBf16)
Def(RGBAf16, fcPixelFormat_RGBAf16)
Def(Rf32, fcPixelFormat_Rf32)
Def(RGf32, fcPixelFormat_RGf32)
Def(RGBf32, fcPixelFormat_RGBf32)
Def(RGBAf32, fcPixelFormat_RGBAf32)
#undef def

template<class T> void CreateVideoData(T *rgba, int width, int height, int frame);
void CreateAudioData(float *samples, int num_samples, double t, float scale);

bool InitializeD3D11();
