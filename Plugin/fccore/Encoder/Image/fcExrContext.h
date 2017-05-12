#pragma once

class fcIExrContext : public fcContextBase
{
public:
    virtual bool beginFrame(const char *path, int width, int height) = 0;
    virtual bool addLayerTexture(void *tex, fcPixelFormat fmt, int channel, const char *name) = 0;
    virtual bool addLayerPixels(const void *pixels, fcPixelFormat fmt, int channel, const char *name) = 0;
    virtual bool endFrame() = 0;
};
fcIExrContext* fcExrCreateContextImpl(const fcExrConfig *conf, fcIGraphicsDevice *dev);
