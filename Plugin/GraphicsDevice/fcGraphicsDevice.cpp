#include "pch.h"
#include "FrameCapturer.h"
#include "fcFoundation.h"
#include "fcGraphicsDevice.h"



fcIGraphicsDevice* fcCreateGraphicsDeviceOpenGL(void *device);
fcIGraphicsDevice* fcCreateGraphicsDeviceD3D9(void *device);
fcIGraphicsDevice* fcCreateGraphicsDeviceD3D11(void *device);


fcIGraphicsDevice *g_the_graphics_device;
fcCLinkage fcExport fcIGraphicsDevice* fcGetGraphicsDevice() { return g_the_graphics_device; }


fcCLinkage fcExport void UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
    if (eventType == kGfxDeviceEventInitialize) {
#ifdef fcSupportD3D9
        if (deviceType == kGfxRendererD3D9)
        {
            g_the_graphics_device = fcCreateGraphicsDeviceD3D9(device);
        }
#endif // fcSupportD3D9
#ifdef fcSupportD3D11
        if (deviceType == kGfxRendererD3D11)
        {
            g_the_graphics_device = fcCreateGraphicsDeviceD3D11(device);
        }
#endif // fcSupportD3D11
#ifdef fcSupportOpenGL
        if (deviceType == kGfxRendererOpenGL)
        {
            g_the_graphics_device = fcCreateGraphicsDeviceOpenGL(device);
        }
#endif // fcSupportOpenGL
    }

    if (eventType == kGfxDeviceEventShutdown) {
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
    UnitySetGraphicsDevice(nullptr, kGfxRendererOpenGL, kGfxDeviceEventInitialize);
}
#endif

#ifdef fcSupportD3D9
fcCLinkage fcExport void fcInitializeD3D9(void *device)
{
    UnitySetGraphicsDevice(device, kGfxRendererD3D9, kGfxDeviceEventInitialize);
}
#endif

#ifdef fcSupportD3D11
fcCLinkage fcExport void fcInitializeD3D11(void *device)
{
    UnitySetGraphicsDevice(device, kGfxRendererD3D11, kGfxDeviceEventInitialize);
}
#endif

fcCLinkage fcExport void fcFinalizeGraphicsDevice()
{
    UnitySetGraphicsDevice(nullptr, kGfxRendererNull, kGfxDeviceEventShutdown);
}

