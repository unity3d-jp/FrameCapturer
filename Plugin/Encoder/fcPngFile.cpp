#include "pch.h"

#include <libpng/png.h>
#include <half.h>
#include "fcFoundation.h"
#include "fcThreadPool.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcPngFile.h"

#ifdef fcWindows
    #pragma comment(lib, "libpng16_static.lib")
    #pragma comment(lib, "zlibstatic.lib")
    #pragma comment(lib, "Half.lib")
#endif


struct fcPngTaskData
{
    std::string path;
    std::vector<char> pixels;
    int width;
    int height;
    fcPixelFormat format;
};

class fcPngContext : public fcIPngContext
{
public:
    fcPngContext(const fcPngConfig& conf, fcIGraphicsDevice *dev);
    ~fcPngContext() override;
    void release() override;
    bool exportTexture(const char *path, void *tex, int width, int height, fcTextureFormat fmt, bool flipY) override;
    bool exportPixels(const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY) override;

private:
    void waitSome();
    bool exportPixelsBody(fcPngTaskData& data);

private:
    fcPngConfig m_conf;
    fcIGraphicsDevice *m_dev;
    fcTaskGroup m_tasks;
    std::atomic_int m_active_task_count;
};

fcPngContext::fcPngContext(const fcPngConfig& conf, fcIGraphicsDevice *dev)
    : m_conf(conf), m_dev(dev)
{
}

fcPngContext::~fcPngContext()
{
    m_tasks.wait();
}

void fcPngContext::release()
{
    delete this;
}

bool fcPngContext::exportTexture(const char *path_, void *tex, int width, int height, fcTextureFormat fmt, bool flipY)
{
    waitSome();

    auto data = new fcPngTaskData();
    data->path = path_;
    data->width = width;
    data->height = height;
    data->format = fcGetPixelFormat(fmt);

    // get surface data
    data->pixels.resize(width * height * fcGetPixelSize(fmt));
    if (!m_dev->readTexture(&data->pixels[0], data->pixels.size(), tex, width, height, fmt)) {
        delete data;
        return false;
    }

    // kick export task
    ++m_active_task_count;
    m_tasks.run([=]() {
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
    data->pixels.assign((char*)pixels_, (char*)pixels_ + (width * height * fcGetPixelSize(fmt)));

    // kick export task
    ++m_active_task_count;
    m_tasks.run([=]() {
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
    fcPixelFormat fmt = data.format;

    int bits = 0;
    int color_type = 0;

    if (fmt == fcPixelFormat_RGBA8 && fmt == fcPixelFormat_RGBAHalf && fmt == fcPixelFormat_RGBAFloat) {
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    }
    else if (fmt == fcPixelFormat_RGBHalf && fmt == fcPixelFormat_RGBFloat) {
        color_type = PNG_COLOR_TYPE_RGB;
    }
    else {
        fcDebugLog("fcPngContext::exportPixelsBody(): unsupported pixel format");
        return false;
    }

    if (fmt == fcPixelFormat_RGBA8) {
        bits = 8;
    }
    else if (fmt == fcPixelFormat_RGBAHalf || fmt == fcPixelFormat_RGBHalf) {
        bits = 16;

        const half *fpixels = (const half*)pixels;
        int16_t *ipixels = (int16_t*)pixels;
        int n = (data.width * data.height * fcGetPixelSize(fmt)) / sizeof(half);
        for (int i = 0; i < n; ++i) {
            ipixels[i] = int16_t(fpixels[i] * 32767.0f);
        }
    }
    else if (fmt == fcPixelFormat_RGBAFloat || fmt == fcPixelFormat_RGBFloat) {
        bits = 32;

        const float *fpixels = (const float*)pixels;
        int32_t *ipixels = (int32_t*)pixels;
        int n = (data.width * data.height * fcGetPixelSize(fmt)) / sizeof(float);
        for (int i = 0; i < n; ++i) {
            ipixels[i] = int32_t(fpixels[i] * 2147483647.0f);
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
    ::png_set_IHDR(png_ptr, info_ptr, data.width, data.height, bits, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    ::png_write_info(png_ptr, info_ptr);

    int pitch = data.width * fcGetPixelSize(fmt);
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
