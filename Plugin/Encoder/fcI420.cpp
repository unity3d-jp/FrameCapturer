#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include <libyuv/libyuv.h>

#pragma comment(lib, "yuv.lib")


void fcI420Image::resize(int width, int height)
{
    int af = roundup<2>(width) * roundup<2>(height);
    y.resize(af);
    u.resize(af >> 2);
    v.resize(af >> 2);
}

fcI420Data fcI420Image::data()
{
    return{ y.data(), u.data(), v.data() };
}


void fcRGBA2I420(fcI420Image& dst, const void *rgba_pixels, int width, int height)
{
    dst.resize(width, height);
    auto data = dst.data();
    libyuv::ABGRToI420(
        (const uint8*)rgba_pixels, width * 4,
        (uint8*)data.y, width,
        (uint8*)data.u, width >> 1,
        (uint8*)data.v, width >> 1,
        width, height);
}
