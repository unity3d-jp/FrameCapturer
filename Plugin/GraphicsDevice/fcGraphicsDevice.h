#include "pch.h"

enum fcGfxDeviceType
{
    fcGfxDeviceType_Unknown,
    fcGfxDeviceType_D3D9,
    fcGfxDeviceType_D3D10,
    fcGfxDeviceType_D3D11,
    fcGfxDeviceType_D3D12,
    fcGfxDeviceType_OpenGL,
    fcGfxDeviceType_Vulkan,
    fcGfxDeviceType_CUDA,
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
fcCLinkage fcExport fcIGraphicsDevice* fcGetGraphicsDevice();
