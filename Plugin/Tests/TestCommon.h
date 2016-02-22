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
#include "../FrameCapturer.h"

union RGBA {
    struct {
        uint8_t r, g, b, a;
    };
    uint8_t v[4];
};

void CreateVideoData(RGBA *rgba, int width, int height, int frame);
void CreateAudioData(float *samples, int num_samples, int frame);

#endif  // TestCommon_h
