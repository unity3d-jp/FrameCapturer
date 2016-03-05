#include "pch.h"
#include "fcFoundation.h"
#include "fcGraphicsDevice.h"



fcIGraphicsDevice* fcCreateGraphicsDeviceOpenGL(void *device);
fcIGraphicsDevice* fcCreateGraphicsDeviceD3D9(void *device);
fcIGraphicsDevice* fcCreateGraphicsDeviceD3D11(void *device);


fcIGraphicsDevice *g_the_graphics_device;
fcCLinkage fcExport fcIGraphicsDevice* fcGetGraphicsDevice() { return g_the_graphics_device; }


fcCLinkage fcExport void UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
    if (eventType == kUnityGfxDeviceEventInitialize) {
#ifdef fcSupportD3D9
        if (deviceType == kUnityGfxRendererD3D9)
        {
            g_the_graphics_device = fcCreateGraphicsDeviceD3D9(device);
        }
#endif // fcSupportD3D9
#ifdef fcSupportD3D11
        if (deviceType == kUnityGfxRendererD3D11)
        {
            g_the_graphics_device = fcCreateGraphicsDeviceD3D11(device);
        }
#endif // fcSupportD3D11
#ifdef fcSupportOpenGL
        if (deviceType == kUnityGfxRendererOpenGL ||
            deviceType == kUnityGfxRendererOpenGLES20 ||
            deviceType == kUnityGfxRendererOpenGLES30 ||
            deviceType == kUnityGfxRendererOpenGLCore )
        {
            g_the_graphics_device = fcCreateGraphicsDeviceOpenGL(device);
        }
#endif // fcSupportOpenGL
    }

    if (eventType == kUnityGfxDeviceEventShutdown) {
        delete g_the_graphics_device;
        g_the_graphics_device = nullptr;
    }
}

fcCLinkage fcExport void UnityRenderEvent(int)
{
}


#ifdef fcSupportOpenGL
fcCLinkage fcExport void fcInitializeOpenGL()
{
    UnitySetGraphicsDevice(nullptr, kUnityGfxRendererOpenGL, kUnityGfxDeviceEventInitialize);
}
#endif

#ifdef fcSupportD3D9
fcCLinkage fcExport void fcInitializeD3D9(void *device)
{
    UnitySetGraphicsDevice(device, kUnityGfxRendererD3D9, kUnityGfxDeviceEventInitialize);
}
#endif

#ifdef fcSupportD3D11
fcCLinkage fcExport void fcInitializeD3D11(void *device)
{
    UnitySetGraphicsDevice(device, kUnityGfxRendererD3D11, kUnityGfxDeviceEventInitialize);
}
#endif

fcCLinkage fcExport void fcFinalizeGraphicsDevice()
{
    UnitySetGraphicsDevice(nullptr, kUnityGfxRendererNull, kUnityGfxDeviceEventShutdown);
}

fcCLinkage fcExport void fcGfxSync()
{
    if (g_the_graphics_device) {
        g_the_graphics_device->sync();
    }
}

