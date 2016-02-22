#include "pch.h"

#include <libpng/png.h>
#include "fcFoundation.h"
#include "fcThreadPool.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcPngFile.h"
#ifdef fcWindows
    #pragma comment(lib, "libpng16_static.lib")
#endif


class fcPngContext : public fcIPngContext
{
public:
    fcPngContext(const fcPngConfig& conf);
    ~fcPngContext();
    void release() override;
    bool exportTexture(const char *path, void *tex, int width, int height, fcTextureFormat fmt, bool flipY) override;
    bool exportPixels(const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY) override;
private:
    fcPngConfig m_conf;
};

fcPngContext::fcPngContext(const fcPngConfig& conf)
    : m_conf(conf)
{
}

fcPngContext::~fcPngContext()
{
}

void fcPngContext::release()
{
    delete this;
}

bool fcPngContext::exportTexture(const char *path, void *tex, int width, int height, fcTextureFormat fmt, bool flipY)
{
    return false;
}

bool fcPngContext::exportPixels(const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY)
{
    return false;
}


fcCLinkage fcExport fcIPngContext* fcPngCreateContext(fcPngConfig *conf)
{
    return new fcPngContext(*conf);
}

fcCLinkage fcExport void fcPngDestroyContext(fcIPngContext *ctx)
{
    ctx->release();
}

fcCLinkage fcExport bool fcPngExportTexture(fcIPngContext *ctx, const char *path, void *tex, int width, int height, fcTextureFormat fmt, bool flipY)
{
    return ctx->exportTexture(path, tex, width, height, fmt, flipY);
}

fcCLinkage fcExport bool fcPngExportPixels(fcIPngContext *ctx, const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY)
{
    return ctx->exportPixels(path, pixels, width, height, fmt, flipY);
}
