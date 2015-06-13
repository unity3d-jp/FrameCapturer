#include "pch.h"
#include "fcColorSpace.h"


static inline int RGBToY(uint8_t r, uint8_t g, uint8_t b)
{
    return (66 * r + 129 * g + 25 * b + 0x1080) >> 8;
}
static inline int RGBToU(uint8_t r, uint8_t g, uint8_t b)
{
    return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static inline int RGBToV(uint8_t r, uint8_t g, uint8_t b)
{
    return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

static inline void RGBAToYRow(uint8_t* dst_y, const bRGBA* src, int width)
{
    for (int x = 0; x < width; ++x) {
        dst_y[0] = RGBToY(src[0].r, src[0].g, src[0].b);
        src += 1;
        dst_y += 1;
    }
}

static void RGBAToUVRow(uint8_t* dst_u, uint8_t* dst_v, const bRGBA* src, int width)
{
    const bRGBA* src1 = src + width;
    for (int x = 0; x < width - 1; x += 2) {
        uint8_t ab = (src[0].b + src[1].b + src1[0].b + src1[1].b) >> 2;
        uint8_t ag = (src[0].g + src[1].g + src1[0].g + src1[1].g) >> 2;
        uint8_t ar = (src[0].r + src[1].r + src1[0].r + src1[1].r) >> 2;
        dst_u[0] = RGBToU(ar, ag, ab);
        dst_v[0] = RGBToV(ar, ag, ab);
        src += 2;
        src1 += 2;
        dst_u += 1;
        dst_v += 1;
    }
    if (width & 1) {
        uint8_t ab = (src[0].b + src1[1].b) >> 1;
        uint8_t ag = (src[0].g + src1[1].g) >> 1;
        uint8_t ar = (src[0].r + src1[1].r) >> 1;
        dst_u[0] = RGBToU(ar, ag, ab);
        dst_v[0] = RGBToV(ar, ag, ab);
    }
}


void RGBA_to_I420(uint8_t* dst_y, uint8_t *dst_u, uint8_t *dst_v, const bRGBA *src, int width, int height)
{
    int stride_uv = width / 2;
    for (int y = 0; y < height - 1; y += 2) {
        RGBAToUVRow(dst_u, dst_v, src, width);
        RGBAToYRow(dst_y, src, width);
        RGBAToYRow(dst_y + width, src + width, width);
        src += width * 2;
        dst_y += width * 2;
        dst_u += stride_uv;
        dst_v += stride_uv;
    }
    if (height & 1) {
        RGBAToUVRow(dst_u, dst_v, src, width);
        RGBAToYRow(dst_y, src, width);
    }
}

void I420_to_RGBA(bRGBA *dst, const uint8_t* src_y, const uint8_t *src_u, const uint8_t *src_v, int width, int height)
{
    // todo:
}
