#include "pch.h"
#include "fcInternal.h"
#include "YUV.h"
#include "Misc.h"

#include <libyuv/libyuv.h>
#ifdef _WIN32
    #pragma comment(lib, "yuv.lib")
#endif


// I420

void I420Image::resize(int width, int height)
{
    int s = roundup<2>(width) * roundup<2>(height);
    int sq = s / 4;
    m_buffer.resize(s + sq + sq);
    m_data.y = m_buffer.data();
    m_data.u = (char*)m_data.y + s;
    m_data.v = (char*)m_data.u + sq;
    m_data.pitch_y = width;
    m_data.pitch_u = m_data.pitch_v = width / 4;
    m_data.height = height;
}

size_t I420Image::size() const
{
    return m_buffer.size();
}
I420Data& I420Image::data()
{
    return m_data;
}
const I420Data& I420Image::data() const
{
    return m_data;
}

void AnyToI420(I420Image& dst, Buffer& tmp, const void *pixels, fcPixelFormat fmt, int width, int height)
{
    if (fmt != fcPixelFormat_RGBAu8 && fmt != fcPixelFormat_RGBu8) {
        tmp.resize(width * height * 4);
        fcConvertPixelFormat(tmp.data(), fcPixelFormat_RGBAu8, pixels, fmt, width * height);
        pixels = tmp.data();
        fmt = fcPixelFormat_RGBAu8;
    }

    dst.resize(width, height);
    auto& data = dst.data();
    if (fmt == fcPixelFormat_RGBAu8) {
        libyuv::ABGRToI420(
            (const uint8*)pixels, width * 4,
            (uint8*)data.y, width,
            (uint8*)data.u, width >> 1,
            (uint8*)data.v, width >> 1,
            width, height);
    }
    else if (fmt == fcPixelFormat_RGBu8) {
        libyuv::RAWToI420(
            (const uint8*)pixels, width * 3,
            (uint8*)data.y, width,
            (uint8*)data.u, width >> 1,
            (uint8*)data.v, width >> 1,
            width, height);
    }
}



// NV12

void NV12Image::resize(int width, int height)
{
    int s = roundup<2>(width) * roundup<2>(height);
    int sh = s / 2;
    m_buffer.resize(s + sh);
    m_data.y = m_buffer.data();
    m_data.uv = (char*)m_data.y + s;
    m_data.pitch_y = width;
    m_data.pitch_uv = width / 2;
    m_data.height = height;
}

size_t NV12Image::size() const
{
    return m_buffer.size();
}
NV12Data& NV12Image::data()
{
    return m_data;
}
const NV12Data& NV12Image::data() const
{
    return m_data;
}

void AnyToNV12(NV12Image& dst, Buffer& tmp, const void *pixels, fcPixelFormat fmt, int width, int height)
{
    if (fmt != fcPixelFormat_RGBAu8) {
        tmp.resize(width * height * 4);
        fcConvertPixelFormat(tmp.data(), fcPixelFormat_RGBAu8, pixels, fmt, width * height);
        pixels = tmp.data();
        fmt = fcPixelFormat_RGBAu8;
    }

    dst.resize(width, height);
    auto& data = dst.data();
    if (fmt == fcPixelFormat_RGBAu8) {
        libyuv::ARGBToNV12(
            (const uint8*)pixels, width * 4,
            (uint8*)data.y, width,
            (uint8*)data.uv, width,
            width, height);
    }
}
