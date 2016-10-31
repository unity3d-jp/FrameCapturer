#include "pch.h"
#include "I420.h"
#include "Misc.h"
#include <libyuv/libyuv.h>

#pragma comment(lib, "yuv.lib")


void I420Image::resize(int width, int height)
{
    int s = roundup<2>(width) * roundup<2>(height);
    m_buffer.resize(s + (s >> 2) + (s >> 2));
    m_data.y = m_buffer.data();
    m_data.u = (char*)m_data.y + s;
    m_data.v = (char*)m_data.u + (s >> 2);
}

size_t I420Image::size() const
{
    return m_buffer.size();
}

const I420Data& I420Image::data() const
{
    return m_data;
}


void RGBA2I420(I420Image& dst, const void *rgba_pixels, int width, int height)
{
    dst.resize(width, height);
    RGBA2I420(dst.data(), rgba_pixels, width, height);
}

void RGBA2I420(const I420Data& dst, const void *rgba_pixels, int width, int height)
{
    libyuv::ABGRToI420(
        (const uint8*)rgba_pixels, width * 4,
        (uint8*)dst.y, width,
        (uint8*)dst.u, width >> 1,
        (uint8*)dst.v, width >> 1,
        width, height);
}
