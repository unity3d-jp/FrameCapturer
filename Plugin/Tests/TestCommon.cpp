#include "TestCommon.h"
#ifdef _WIN32
    #pragma comment(lib, "Half.lib")
#endif


template<class T> TRGBA<T> White();
template<class T> TRGBA<T> Black();

template<> TRGBA<u8>    White() { return TRGBA<u8>(255, 255, 255, 255); }
template<> TRGBA<f16>   White() { return TRGBA<f16>(1.0f, 1.0f, 1.0f, 1.0f); }
template<> TRGBA<f32>   White() { return TRGBA<f32>(1.0f, 1.0f, 1.0f, 1.0f); }

template<> TRGBA<u8>    Black() { return TRGBA<u8>(0, 0, 0, 255); }
template<> TRGBA<f16>   Black() { return TRGBA<f16>(0.0f, 0.0f, 0.0f, 1.0f); }
template<> TRGBA<f32>   Black() { return TRGBA<f32>(0.0f, 0.0f, 0.0f, 1.0f); }


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
                rgba[ip] = White<T>();
            }
            else {
                rgba[ip] = Black<T>();
            }
        }
    }
}
template void CreateVideoData< u8>(TRGBA< u8> *rgba, int width, int height, int frame);
template void CreateVideoData<f16>(TRGBA<f16> *rgba, int width, int height, int frame);
template void CreateVideoData<f32>(TRGBA<f32> *rgba, int width, int height, int frame);


void CreateAudioData(float *samples, int num_samples, int frame)
{
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = std::sin((float(i + (num_samples * frame)) * 0.5f) * (3.14159f / 180.0f)) * 32767.0f;
    }
}
