#include "pch.h"
#include "YUV.h"
#include "Misc.h"
#include "fcFoundation.h"

#include <libyuv/libyuv.h>
#ifdef _WIN32
    #pragma comment(lib, "yuv.lib")
#endif


// I420

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


void RGBAToI420(I420Image& dst, const void *rgba_pixels, int width, int height)
{
    dst.resize(width, height);
    RGBAToI420(dst.data(), rgba_pixels, width, height);
}

void RGBAToI420(const I420Data& dst, const void *rgba_pixels, int width, int height)
{
    libyuv::ABGRToI420(
        (const uint8*)rgba_pixels, width * 4,
        (uint8*)dst.y, width,
        (uint8*)dst.u, width >> 1,
        (uint8*)dst.v, width >> 1,
        width, height);
}

void AnyToI420(I420Image& dst, Buffer& tmp, const void *pixels, fcPixelFormat fmt, int width, int height)
{
    if (fmt != fcPixelFormat_RGBAu8) {
        tmp.resize(width * height * 4);
        fcConvertPixelFormat(tmp.data(), fcPixelFormat_RGBAu8, pixels, fmt, width * height);
        pixels = tmp.data();
        fmt = fcPixelFormat_RGBAu8;
    }
    RGBAToI420(dst, pixels, width, height);
}



// NV12

void NV12Image::resize(int width, int height)
{
    int s = roundup<2>(width) * roundup<2>(height);
    m_buffer.resize(s + (s >> 1));
    m_data.y = m_buffer.data();
    m_data.uv = (char*)m_data.y + s;
}

size_t NV12Image::size() const
{
    return m_buffer.size();
}

const NV12Data& NV12Image::data() const
{
    return m_data;
}

void RGBAToNV12(NV12Image& dst, const void *rgba_pixels, int width, int height)
{
    dst.resize(width, height);
    RGBAToNV12(dst.data(), rgba_pixels, width, height);
}

void RGBAToNV12(const NV12Data& dst, const void *rgba_pixels, int width, int height)
{
    libyuv::ARGBToNV12(
        (const uint8*)rgba_pixels, width * 4,
        (uint8*)dst.y, width,
        (uint8*)dst.uv, width,
        width, height);
}

void AnyToNV12(NV12Image& dst, Buffer& tmp, const void *pixels, fcPixelFormat fmt, int width, int height)
{
    if (fmt != fcPixelFormat_RGBAu8) {
        tmp.resize(width * height * 4);
        fcConvertPixelFormat(tmp.data(), fcPixelFormat_RGBAu8, pixels, fmt, width * height);
        pixels = tmp.data();
        fmt = fcPixelFormat_RGBAu8;
    }
    RGBAToNV12(dst, pixels, width, height);
}
