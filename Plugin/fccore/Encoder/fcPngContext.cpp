#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcPngContext.h"

#include <libpng/png.h>
#ifdef fcWindows
    #pragma comment(lib, "libpng16_static.lib")
    #pragma comment(lib, "zlibstatic.lib")
#endif


struct fcPngTaskData
{
    std::string path;
    Buffer pixels;
    Buffer buf; // buffer for conversion
    int width = 0;
    int height = 0;
    fcPixelFormat format = fcPixelFormat_Unknown;
    bool flipY = false;
};

class fcPngContext : public fcIPngContext
{
public:
    fcPngContext(const fcPngConfig& conf, fcIGraphicsDevice *dev);
    ~fcPngContext() override;
    void release() override;
    bool exportTexture(const char *path, void *tex, int width, int height, fcPixelFormat fmt, bool flipY) override;
    bool exportPixels(const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY) override;

private:
    void waitSome();
    bool exportPixelsBody(fcPngTaskData& data);

private:
    fcPngConfig m_conf;
    fcIGraphicsDevice *m_dev = nullptr;
    fcTaskGroup m_tasks;
    std::atomic_int m_active_task_count = 0;
};

fcPngContext::fcPngContext(const fcPngConfig& conf, fcIGraphicsDevice *dev)
    : m_conf(conf)
    , m_dev(dev)
{
    if (m_conf.max_active_tasks <= 0) {
        m_conf.max_active_tasks = std::thread::hardware_concurrency();
    }
}

fcPngContext::~fcPngContext()
{
    m_tasks.wait();
}

void fcPngContext::release()
{
    delete this;
}

bool fcPngContext::exportTexture(const char *path_, void *tex, int width, int height, fcPixelFormat fmt, bool flipY)
{
    if (m_dev == nullptr) {
        fcDebugLog("fcPngContext::exportTexture(): gfx device is null.");
        return false;
    }
    waitSome();

    auto data = new fcPngTaskData();
    data->path = path_;
    data->width = width;
    data->height = height;
    data->format = fmt;
    data->flipY = flipY;

    // get surface data
    data->pixels.resize(width * height * fcGetPixelSize(fmt));
    if (!m_dev->readTexture(&data->pixels[0], data->pixels.size(), tex, width, height, fmt)) {
        delete data;
        return false;
    }

    // kick export task
    ++m_active_task_count;
    m_tasks.run([this, data]() {
        exportPixelsBody(*data);
        delete data;
        --m_active_task_count;
    });

    return false;
}

bool fcPngContext::exportPixels(const char *path_, const void *pixels_, int width, int height, fcPixelFormat fmt, bool flipY)
{
    waitSome();

    auto data = new fcPngTaskData();
    data->path = path_;
    data->width = width;
    data->height = height;
    data->format = fmt;
    data->flipY = flipY;
    data->pixels.assign((char*)pixels_, width * height * fcGetPixelSize(fmt));

    // kick export task
    ++m_active_task_count;
    m_tasks.run([this, data]() {
        exportPixelsBody(*data);
        delete data;
        --m_active_task_count;
    });
    return true;
}

void fcPngContext::waitSome()
{
    if (m_active_task_count >= m_conf.max_active_tasks) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (m_active_task_count >= m_conf.max_active_tasks) {
            m_tasks.wait();
        }
    }
}

bool fcPngContext::exportPixelsBody(fcPngTaskData& data)
{
    png_bytep pixels = (png_bytep)&data.pixels[0];

    int npixels = data.width * data.height;
    int bit_depth = 0;
    int num_channels = 0;
    int color_type = 0;

    if (data.flipY) {
        fcImageFlipY(&data.pixels[0], data.width, data.height, data.format);
    }

    if (m_conf.pixel_format == fcPngPixelFormat::UInt8) {
        switch (data.format) {
            // u8
        case fcPixelFormat_RGBAu8:
            bit_depth = 8;
            num_channels = 4;
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
            break;
        case fcPixelFormat_RGBu8:
            bit_depth = 8;
            num_channels = 3;
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case fcPixelFormat_RGu8:
            data.buf.resize(npixels * 3);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_RGBu8, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 8;
            num_channels = 3;
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case fcPixelFormat_Ru8:
            bit_depth = 8;
            num_channels = 1;
            color_type = PNG_COLOR_TYPE_GRAY;
            break;

        case fcPixelFormat_RGBAf32:
        case fcPixelFormat_RGBAf16:
            data.buf.resize(npixels * 4);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_RGBAu8, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 8;
            num_channels = 4;
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
            break;
        case fcPixelFormat_RGBf32:
        case fcPixelFormat_RGf32:
        case fcPixelFormat_RGBf16:
        case fcPixelFormat_RGf16:
            data.buf.resize(npixels * 3);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_RGBu8, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 8;
            num_channels = 3;
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case fcPixelFormat_Rf32:
        case fcPixelFormat_Rf16:
            data.buf.resize(npixels * 1);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_Ru8, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 8;
            num_channels = 1;
            color_type = PNG_COLOR_TYPE_GRAY;
            break;

        default:
            fcDebugLog("fcPngContext::exportPixelsBody(): unsupported pixel format");
            return false;
        }
    }
    else if (m_conf.pixel_format == fcPngPixelFormat::UInt16) {
        switch (data.format) {
        case fcPixelFormat_RGBAf32:
        case fcPixelFormat_RGBAf16:
        case fcPixelFormat_RGBAu8:
            data.buf.resize(npixels * 8);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_RGBAi16, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 16;
            num_channels = 4;
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
            break;
        case fcPixelFormat_RGBf32:
        case fcPixelFormat_RGf32:
        case fcPixelFormat_RGBf16:
        case fcPixelFormat_RGf16:
        case fcPixelFormat_RGBu8:
        case fcPixelFormat_RGu8:
            data.buf.resize(npixels * 6);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_RGBi16, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 16;
            num_channels = 3;
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case fcPixelFormat_Rf32:
        case fcPixelFormat_Rf16:
        case fcPixelFormat_Ru8:
            data.buf.resize(npixels * 2);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_Ri16, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 16;
            num_channels = 1;
            color_type = PNG_COLOR_TYPE_GRAY;
            break;
        default:
            fcDebugLog("fcPngContext::exportPixelsBody(): unsupported pixel format");
            return false;
        }
    }
    else { // adaptive
        switch (data.format) {
            // u8
        case fcPixelFormat_RGBAu8:
            bit_depth = 8;
            num_channels = 4;
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
            break;
        case fcPixelFormat_RGBu8:
            bit_depth = 8;
            num_channels = 3;
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case fcPixelFormat_RGu8:
            data.buf.resize(npixels * 3);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_RGBu8, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 8;
            num_channels = 3;
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case fcPixelFormat_Ru8:
            bit_depth = 8;
            num_channels = 1;
            color_type = PNG_COLOR_TYPE_GRAY;
            break;

            // float/half -> i16
        case fcPixelFormat_RGBAf32:
        case fcPixelFormat_RGBAf16:
            data.buf.resize(npixels * 8);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_RGBAi16, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 16;
            num_channels = 4;
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
            break;
        case fcPixelFormat_RGBf32:
        case fcPixelFormat_RGf32:
        case fcPixelFormat_RGBf16:
        case fcPixelFormat_RGf16:
            data.buf.resize(npixels * 6);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_RGBi16, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 16;
            num_channels = 3;
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case fcPixelFormat_Rf32:
        case fcPixelFormat_Rf16:
            data.buf.resize(npixels * 2);
            fcConvertPixelFormat(&data.buf[0], fcPixelFormat_Ri16, &data.pixels[0], data.format, npixels);
            pixels = (png_bytep)&data.buf[0];
            bit_depth = 16;
            num_channels = 1;
            color_type = PNG_COLOR_TYPE_GRAY;
            break;

        default:
            fcDebugLog("fcPngContext::exportPixelsBody(): unsupported pixel format");
            return false;
        }
    }

    png_structp png_ptr = ::png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (png_ptr == nullptr) {
        fcDebugLog("fcPngContext::exportPixelsBody(): png_create_write_struct() returned nullptr");
        return false;
    }

    png_infop info_ptr = ::png_create_info_struct(png_ptr);
    if (info_ptr == nullptr) {
        fcDebugLog("fcPngContext::exportPixelsBody(): png_create_info_struct() returned nullptr");
        ::png_destroy_write_struct(&png_ptr, nullptr);
        return false;
    }

    FILE *ofile = ::fopen(data.path.c_str(), "wb");
    if (ofile == nullptr) {
        fcDebugLog("fcPngContext::exportPixelsBody(): file open failed");
        ::png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    ::png_init_io(png_ptr, ofile);
    ::png_set_IHDR(png_ptr, info_ptr, data.width, data.height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    ::png_write_info(png_ptr, info_ptr);

    int pitch = data.width * (bit_depth / 8) * num_channels;
    std::vector<png_bytep> row_pointers(data.height);
    for (int yi = 0; yi <data.height; ++yi) {
        row_pointers[yi] = &pixels[pitch * yi];
    }

    ::png_write_image(png_ptr, &row_pointers[0]);
    ::png_write_end(png_ptr, info_ptr);

    ::fclose(ofile);
    ::png_destroy_write_struct(&png_ptr, &info_ptr);

    return true;
}

fcIPngContext* fcPngCreateContextImpl(const fcPngConfig *conf, fcIGraphicsDevice *dev)
{
    fcPngConfig default_cont;
    if (conf == nullptr) { conf = &default_cont; }
    return new fcPngContext(*conf, dev);
}
