#include "TestCommon.h"
#ifdef _WIN32
    #pragma comment(lib, "Half.lib")
#endif


template<class T> T White();
template<class T> T Black();

template<> RGBu8     White() { return RGBu8(255, 255, 255); }
template<> RGBf16    White() { return RGBf16(1.0f, 1.0f, 1.0f); }
template<> RGBf32    White() { return RGBf32(1.0f, 1.0f, 1.0f); }
template<> RGBAu8    White() { return RGBAu8(255, 255, 255, 255); }
template<> RGBAf16   White() { return RGBAf16(1.0f, 1.0f, 1.0f, 1.0f); }
template<> RGBAf32   White() { return RGBAf32(1.0f, 1.0f, 1.0f, 1.0f); }

template<> RGBu8     Black() { return RGBu8(0, 0, 0); }
template<> RGBf16    Black() { return RGBf16(0.0f, 0.0f, 0.0f); }
template<> RGBf32    Black() { return RGBf32(0.0f, 0.0f, 0.0f); }
template<> RGBAu8    Black() { return RGBAu8(0, 0, 0, 255); }
template<> RGBAf16   Black() { return RGBAf16(0.0f, 0.0f, 0.0f, 1.0f); }
template<> RGBAf32   Black() { return RGBAf32(0.0f, 0.0f, 0.0f, 1.0f); }


template<class T>
void CreateVideoData(T *pixels, int width, int height, int frame)
{
    const int block_size = 32;
    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            int ip = iy * width + ix;
            int yb = iy / block_size;
            int xb = (ix + iy + frame) / block_size;

            if ((xb) % 2 == 0) {
                pixels[ip] = White<T>();
            }
            else {
                pixels[ip] = Black<T>();
            }
        }
    }
}

template void CreateVideoData<  RGBu8>(  RGBu8 *pixels, int width, int height, int frame);
template void CreateVideoData< RGBf16>( RGBf16 *pixels, int width, int height, int frame);
template void CreateVideoData< RGBf32>( RGBf32 *pixels, int width, int height, int frame);
template void CreateVideoData< RGBAu8>( RGBAu8 *pixels, int width, int height, int frame);
template void CreateVideoData<RGBAf16>(RGBAf16 *pixels, int width, int height, int frame);
template void CreateVideoData<RGBAf32>(RGBAf32 *pixels, int width, int height, int frame);


void CreateAudioData(float *samples, int num_samples, int frame)
{
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = std::sin((float(i + (num_samples * frame)) * 0.5f) * (3.14159f / 180.0f)) * 32767.0f;
    }
}
