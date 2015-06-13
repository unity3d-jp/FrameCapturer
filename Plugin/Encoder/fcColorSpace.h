#ifndef fcColorSpace_h
#define fcColorSpace_h

#include <cstdint>

template<class T>
struct RGBA
{
    T r, g, b, a;
};
typedef RGBA<uint8_t> bRGBA;


void RGBA_to_I420(uint8_t* dst_y, uint8_t *dst_u, uint8_t *dst_v, const bRGBA *src, int width, int height);
void I420_to_RGBA(bRGBA *dst, const uint8_t* src_y, const uint8_t *src_u, const uint8_t *src_v, int width, int height);

#endif // fcColorSpace_h
