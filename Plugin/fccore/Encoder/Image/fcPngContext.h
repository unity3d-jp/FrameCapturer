#pragma once

class fcIPngContext : public fcContextBase
{
public:
    virtual bool exportTexture(const char *path, void *tex, int width, int height, fcPixelFormat fmt, int num_channels) = 0;
    virtual bool exportPixels(const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, int num_channels) = 0;
};

fcIPngContext* fcPngCreateContextImpl(const fcPngConfig *conf, fcIGraphicsDevice *dev);