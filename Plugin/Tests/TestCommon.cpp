#include "TestCommon.h"
#ifdef _WIN32
    #pragma comment(lib, "Half.lib")
#endif


template<class T> T White();
template<class T> T Black();

template<> RGB     White() { return RGB(255, 255, 255); }
template<> hRGB    White() { return hRGB(1.0f, 1.0f, 1.0f); }
template<> fRGB    White() { return fRGB(1.0f, 1.0f, 1.0f); }
template<> RGBA    White() { return RGBA(255, 255, 255, 255); }
template<> hRGBA   White() { return hRGBA(1.0f, 1.0f, 1.0f, 1.0f); }
template<> fRGBA   White() { return fRGBA(1.0f, 1.0f, 1.0f, 1.0f); }

template<> RGB     Black() { return RGB(0, 0, 0); }
template<> hRGB    Black() { return hRGB(0.0f, 0.0f, 0.0f); }
template<> fRGB    Black() { return fRGB(0.0f, 0.0f, 0.0f); }
template<> RGBA    Black() { return RGBA(0, 0, 0, 255); }
template<> hRGBA   Black() { return hRGBA(0.0f, 0.0f, 0.0f, 1.0f); }
template<> fRGBA   Black() { return fRGBA(0.0f, 0.0f, 0.0f, 1.0f); }


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

template void CreateVideoData<  RGB>(  RGB *pixels, int width, int height, int frame);
template void CreateVideoData< hRGB>( hRGB *pixels, int width, int height, int frame);
template void CreateVideoData< fRGB>( fRGB *pixels, int width, int height, int frame);
template void CreateVideoData< RGBA>( RGBA *pixels, int width, int height, int frame);
template void CreateVideoData<hRGBA>(hRGBA *pixels, int width, int height, int frame);
template void CreateVideoData<fRGBA>(fRGBA *pixels, int width, int height, int frame);


void CreateAudioData(float *samples, int num_samples, int frame)
{
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = std::sin((float(i + (num_samples * frame)) * 0.5f) * (3.14159f / 180.0f)) * 32767.0f;
    }
}
