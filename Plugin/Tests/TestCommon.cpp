#include "TestCommon.h"

void CreateVideoData(RGBA *rgba, int width, int height, int frame)
{
    const int block_size = 32;
    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            int ip = iy * width + ix;
            int yb = iy / block_size;
            int xb = (ix + iy + frame) / block_size;

            if ((xb) % 2 == 0) {
                rgba[ip].r = rgba[ip].g = rgba[ip].b = rgba[ip].a = 255;
            }
            else {
                rgba[ip].r = rgba[ip].g = rgba[ip].b = rgba[ip].a = 0;
            }
        }
    }
}

// audio data generator

void CreateAudioData(float *samples, int num_samples, int frame)
{
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = std::sin((float(i + (num_samples * frame)) * 0.5f) * (3.14159f / 180.0f)) * 32767.0f;
    }
}
