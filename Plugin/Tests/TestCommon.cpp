#include "TestCommon.h"
#ifdef _WIN32
    #pragma comment(lib, "Half.lib")
#endif


template<class T> TRGBA<T> White();
template<class T> TRGBA<T> Black() { return TRGBA<T>(); }

template<> TRGBA<uint8_t> White() { return TRGBA<uint8_t>(255, 255, 255, 255); }
template<> TRGBA<half>    White() { return TRGBA<half>(1.0f, 1.0f, 1.0f, 1.0f); }
template<> TRGBA<float>   White() { return TRGBA<float>(1.0f, 1.0f, 1.0f, 1.0f); }


template<class T>
void CreateVideoData(TRGBA<T> *rgba, int width, int height, int frame)
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
template void CreateVideoData<uint8_t>(TRGBA<uint8_t> *rgba, int width, int height, int frame);
template void CreateVideoData<half>(TRGBA<half> *rgba, int width, int height, int frame);
template void CreateVideoData<float>(TRGBA<float> *rgba, int width, int height, int frame);


void CreateAudioData(float *samples, int num_samples, int frame)
{
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = std::sin((float(i + (num_samples * frame)) * 0.5f) * (3.14159f / 180.0f)) * 32767.0f;
    }
}
