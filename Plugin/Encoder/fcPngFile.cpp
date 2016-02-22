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


class fcPngContext : public fcIPngContext
{
public:
    fcPngContext(const fcPngConfig& conf, fcIGraphicsDevice *dev);
    ~fcPngContext() override;
    void release() override;
    bool exportTexture(const char *path, void *tex, int width, int height, fcTextureFormat fmt, bool flipY) override;
    bool exportPixels(const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY) override;

private:
    bool exportPixelsBody(const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY);

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
    std::string path = path_;

    return false;
}

bool fcPngContext::exportPixels(const char *path_, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY)
{
    std::string path = path_;

    ++m_active_task_count;
    m_tasks.run([=]() {
        exportPixelsBody(path.c_str(), pixels, width, height, fmt, flipY);
        --m_active_task_count;
    });
    return true;
}

bool fcPngContext::exportPixelsBody(const char *path, const void *pixels_, int width, int height, fcPixelFormat fmt, bool flipY)
{
    png_structp png_ptr = ::png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (png_ptr == nullptr)
    {
        fcDebugLog("fcPngContext::exportPixelsBody(): png_create_write_struct() returned nullptr");
        return false;
    }

    png_infop info_ptr = ::png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
    {
        fcDebugLog("fcPngContext::exportPixelsBody(): png_create_info_struct() returned nullptr");
        ::png_destroy_write_struct(&png_ptr, nullptr);
        return false;
    }

    FILE *ofile = fopen(path, "wb");
    if (ofile == nullptr) {
        fcDebugLog("fcPngContext::exportPixelsBody(): file opend failed");
        ::png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    ::png_init_io(png_ptr, ofile);
    ::png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    ::png_write_info(png_ptr, info_ptr);

    int pitch = width * fcGetPixelSize(fmt);
    png_bytep pixels = (png_bytep)pixels_;
    std::vector<png_bytep> row_pointers(height);
    for (int yi = 0; yi <height; ++yi) {
        row_pointers[yi] = &pixels[pitch * yi];
    }

    ::png_write_image(png_ptr, &row_pointers[0]);
    ::png_write_end(png_ptr, info_ptr);

    ::fclose(ofile);
    ::png_destroy_write_struct(&png_ptr, &info_ptr);

    return true;
}
