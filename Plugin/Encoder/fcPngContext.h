#pragma once

class fcIPngContext
{
public:
    virtual void release() = 0;
    virtual bool exportTexture(const char *path, void *tex, int width, int height, fcPixelFormat fmt, bool flipY) = 0;
    virtual bool exportPixels(const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY) = 0;
protected:
    virtual ~fcIPngContext() {}
};
typedef fcIPngContext* (*fcPngCreateContextImplT)(const fcPngConfig *conf, fcIGraphicsDevice*);
