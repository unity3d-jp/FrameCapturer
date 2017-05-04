#include "pch.h"

enum class fcGfxDeviceType
{
    Unknown,
    D3D9,
    D3D10,
    D3D11,
    D3D12,
    OpenGL,
    Vulkan,
    CUDA,
};


class fcIGraphicsDevice
{
public:
    virtual ~fcIGraphicsDevice() {}
    virtual void* getDevicePtr() = 0;
    virtual fcGfxDeviceType getDeviceType() = 0;
    virtual void sync() = 0;
    virtual bool readTexture(void *o_buf, size_t bufsize, void *tex, int width, int height, fcPixelFormat format) = 0;
    virtual bool writeTexture(void *o_tex, int width, int height, fcPixelFormat format, const void *buf, size_t bufsize) = 0;
};
fcAPI fcIGraphicsDevice* fcGetGraphicsDevice();
