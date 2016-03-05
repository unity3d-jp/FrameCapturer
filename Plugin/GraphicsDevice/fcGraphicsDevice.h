#include "pch.h"


typedef enum UnityGfxRenderer
{
    kUnityGfxRendererOpenGL            =  0, // Legacy OpenGL
    kUnityGfxRendererD3D9              =  1, // Direct3D 9
    kUnityGfxRendererD3D11             =  2, // Direct3D 11
    kUnityGfxRendererGCM               =  3, // PlayStation 3
    kUnityGfxRendererNull              =  4, // "null" device (used in batch mode)
    kUnityGfxRendererXenon             =  6, // Xbox 360
    kUnityGfxRendererOpenGLES20        =  8, // OpenGL ES 2.0
    kUnityGfxRendererOpenGLES30        = 11, // OpenGL ES 3.0
    kUnityGfxRendererGXM               = 12, // PlayStation Vita
    kUnityGfxRendererPS4               = 13, // PlayStation 4
    kUnityGfxRendererXboxOne           = 14, // Xbox One        
    kUnityGfxRendererMetal             = 16, // iOS Metal
    kUnityGfxRendererOpenGLCore        = 17, // OpenGL core
    kUnityGfxRendererD3D12             = 18, // Direct3D 12
} UnityGfxRenderer;

typedef enum UnityGfxDeviceEventType
{
    kUnityGfxDeviceEventInitialize     = 0,
    kUnityGfxDeviceEventShutdown       = 1,
    kUnityGfxDeviceEventBeforeReset    = 2,
    kUnityGfxDeviceEventAfterReset     = 3,
} UnityGfxDeviceEventType;


class fcIGraphicsDevice
{
public:
    virtual ~fcIGraphicsDevice() {}
    virtual void* getDevicePtr() = 0;
    virtual int getDeviceType() = 0;
    virtual void sync() = 0;
    virtual bool readTexture(void *o_buf, size_t bufsize, void *tex, int width, int height, fcPixelFormat format) = 0;
    virtual bool writeTexture(void *o_tex, int width, int height, fcPixelFormat format, const void *buf, size_t bufsize) = 0;
};
fcCLinkage fcExport fcIGraphicsDevice* fcGetGraphicsDevice();
