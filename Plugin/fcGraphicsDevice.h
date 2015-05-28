#include "pch.h"


// Graphics device identifiers in Unity
enum GfxDeviceRenderer
{
    kGfxRendererOpenGL = 0,          // OpenGL
    kGfxRendererD3D9,                // Direct3D 9
    kGfxRendererD3D11,               // Direct3D 11
    kGfxRendererGCM,                 // Sony PlayStation 3 GCM
    kGfxRendererNull,                // "null" device (used in batch mode)
    kGfxRendererHollywood,           // Nintendo Wii
    kGfxRendererXenon,               // Xbox 360
    kGfxRendererOpenGLES,            // OpenGL ES 1.1
    kGfxRendererOpenGLES20Mobile,    // OpenGL ES 2.0 mobile variant
    kGfxRendererMolehill,            // Flash 11 Stage3D
    kGfxRendererOpenGLES20Desktop,   // OpenGL ES 2.0 desktop variant (i.e. NaCl)
    kGfxRendererCount
};

// Event types for UnitySetGraphicsDevice
enum GfxDeviceEventType {
    kGfxDeviceEventInitialize = 0,
    kGfxDeviceEventShutdown,
    kGfxDeviceEventBeforeReset,
    kGfxDeviceEventAfterReset,
};


class fcGraphicsDevice
{
public:
    virtual ~fcGraphicsDevice() {}
    virtual void* createTmpTexture(int width, int height, fcETextureFormat format) = 0;
    virtual void releaseTmpTexture(void *tex) = 0;
    virtual bool copyTextureData(void *o_data, void *tex, void *tmp, int width, int height, int format) = 0;

    virtual void* getDevicePtr() = 0;
    virtual int getDeviceType() = 0;
};
fcCLinkage fcExport fcGraphicsDevice* fcGetGraphicsDevice();

