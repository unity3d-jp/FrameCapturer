#include "TestCommon.h"
#ifdef _WIN32
    #pragma comment(lib, "Half.lib")
#endif


template<class T> T White();
template<class T> T Black();

template<> Ru8       White() { return Ru8(255); }
template<> Rf16      White() { return Rf16(1.0f); }
template<> Rf32      White() { return Rf32(1.0f); }
template<> RGu8      White() { return RGu8(255, 255); }
template<> RGf16     White() { return RGf16(1.0f, 1.0f); }
template<> RGf32     White() { return RGf32(1.0f, 1.0f); }
template<> RGBu8     White() { return RGBu8(255, 255, 255); }
template<> RGBf16    White() { return RGBf16(1.0f, 1.0f, 1.0f); }
template<> RGBf32    White() { return RGBf32(1.0f, 1.0f, 1.0f); }
template<> RGBAu8    White() { return RGBAu8(255, 255, 255, 255); }
template<> RGBAf16   White() { return RGBAf16(1.0f, 1.0f, 1.0f, 1.0f); }
template<> RGBAf32   White() { return RGBAf32(1.0f, 1.0f, 1.0f, 1.0f); }

template<> Ru8       Black() { return Ru8(0); }
template<> Rf16      Black() { return Rf16(0.0f); }
template<> Rf32      Black() { return Rf32(0.0f); }
template<> RGu8      Black() { return RGu8(0, 0); }
template<> RGf16     Black() { return RGf16(0.0f, 0.0f); }
template<> RGf32     Black() { return RGf32(0.0f, 0.0f); }
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

template void CreateVideoData<   Ru8 >(Ru8 *pixels, int width, int height, int frame);
template void CreateVideoData<   Rf16>(Rf16 *pixels, int width, int height, int frame);
template void CreateVideoData<   Rf32>(Rf32 *pixels, int width, int height, int frame);
template void CreateVideoData<  RGu8 >(RGu8 *pixels, int width, int height, int frame);
template void CreateVideoData<  RGf16>(RGf16 *pixels, int width, int height, int frame);
template void CreateVideoData<  RGf32>(RGf32 *pixels, int width, int height, int frame);
template void CreateVideoData< RGBu8 >(RGBu8 *pixels, int width, int height, int frame);
template void CreateVideoData< RGBf16>(RGBf16 *pixels, int width, int height, int frame);
template void CreateVideoData< RGBf32>(RGBf32 *pixels, int width, int height, int frame);
template void CreateVideoData<RGBAu8 >(RGBAu8 *pixels, int width, int height, int frame);
template void CreateVideoData<RGBAf16>(RGBAf16 *pixels, int width, int height, int frame);
template void CreateVideoData<RGBAf32>(RGBAf32 *pixels, int width, int height, int frame);


void CreateAudioData(float *samples, int num_samples, double t, float scale)
{
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = std::sin((float(i + ((double)num_samples * t)) * 5.5f) * (3.14159f / 180.0f)) * scale;
    }
}
