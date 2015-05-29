#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"


int fcGetPixelSize(fcETextureFormat format)
{
    switch (format)
    {
    case fcE_ARGB32:    return 4;

    case fcE_ARGBHalf:  return 8;
    case fcE_RGHalf:    return 4;
    case fcE_RHalf:     return 2;

    case fcE_ARGBFloat: return 16;
    case fcE_RGFloat:   return 8;
    case fcE_RFloat:    return 4;

    case fcE_ARGBInt:   return 16;
    case fcE_RGInt:     return 8;
    case fcE_RInt:      return 4;
    }
    return 0;
}


fcGraphicsDevice* fcCreateGraphicsDeviceOpenGL(void *device);
fcGraphicsDevice* fcCreateGraphicsDeviceD3D9(void *device);
fcGraphicsDevice* fcCreateGraphicsDeviceD3D11(void *device);


fcGraphicsDevice *g_the_graphics_device;
fcCLinkage fcExport fcGraphicsDevice* fcGetGraphicsDevice() { return g_the_graphics_device; }
typedef fcGraphicsDevice* (*fcGetGraphicsDeviceT)();


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

fcCLinkage fcExport void UnityRenderEvent(int eventID)
{
}




#if !defined(fcMaster) && defined(fcWindows)

// PatchLibrary で突っ込まれたモジュールは UnitySetGraphicsDevice() が呼ばれないので、
// DLL_PROCESS_ATTACH のタイミングで先にロードされているモジュールからデバイスをもらって同等の処理を行う。
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call == DLL_PROCESS_ATTACH)
    {
        HMODULE m = ::GetModuleHandleA("FrameCapturer.dll");
        if (m) {
            auto proc = (fcGetGraphicsDeviceT)::GetProcAddress(m, "fcGetGraphicsDevice");
            if (proc) {
                fcGraphicsDevice *dev = proc();
                if (dev) {
                    UnitySetGraphicsDevice(dev->getDevicePtr(), dev->getDeviceType(), kGfxDeviceEventInitialize);
                }
            }
        }
    }
    else if (reason_for_call == DLL_PROCESS_DETACH)
    {
    }
    return TRUE;
}

// "DllMain already defined in MSVCRT.lib" 対策
#ifdef _X86_
extern "C" { int _afxForceUSRDLL; }
#else
extern "C" { int __afxForceUSRDLL; }
#endif

#endif
