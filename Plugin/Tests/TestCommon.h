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

template<class T>
union TRGBA {
    struct {
        T r, g, b, a;
    };
    T v[4];

    TRGBA() { memset(this, 0, sizeof(*this)); }
    TRGBA(T a, T b, T c, T d) { r = a; g = b; b = c; a = d; }
};
typedef TRGBA<uint8_t> RGBA;

template<class T> void CreateVideoData(TRGBA<T> *rgba, int width, int height, int frame);
void CreateAudioData(float *samples, int num_samples, int frame);

#endif  // TestCommon_h
