#include "pch.h"
#include "fcFoundation.h"
#include "fcGraphicsDevice.h"



fcIGraphicsDevice* fcCreateGraphicsDeviceOpenGL();
fcIGraphicsDevice* fcCreateGraphicsDeviceD3D9(void *device);
fcIGraphicsDevice* fcCreateGraphicsDeviceD3D11(void *device);


static fcIGraphicsDevice *g_gfx_device;
fcCLinkage fcExport fcIGraphicsDevice* fcGetGraphicsDevice() { return g_gfx_device; }


fcCLinkage fcExport void fcGfxInitializeOpenGL()
{
#ifdef fcSupportOpenGL
    if (g_gfx_device != nullptr) {
        fcDebugLog("fcInitializeOpenGL(): already initialized");
        return;
    }
    g_gfx_device = fcCreateGraphicsDeviceOpenGL();
#endif
}

fcCLinkage fcExport void fcGfxInitializeD3D9(void *device)
{
#ifdef fcSupportD3D9
    if (g_gfx_device != nullptr) {
        fcDebugLog("fcInitializeD3D9(): already initialized");
        return;
    }
    g_gfx_device = fcCreateGraphicsDeviceD3D9(device);
#endif
}

fcCLinkage fcExport void fcGfxInitializeD3D11(void *device)
{
#ifdef fcSupportD3D11
    if (g_gfx_device != nullptr) {
        fcDebugLog("fcInitializeD3D11(): already initialized");
        return;
    }
    g_gfx_device = fcCreateGraphicsDeviceD3D11(device);
#endif
}

fcCLinkage fcExport void fcGfxFinalize()
{
    delete g_gfx_device;
    g_gfx_device = nullptr;
}

fcCLinkage fcExport void fcGfxSync()
{
    if (g_gfx_device) {
        g_gfx_device->sync();
    }
}



#ifndef fcStaticLink

#include "PluginAPI/IUnityGraphics.h"
#ifdef fcSupportD3D9
    #include <d3d9.h>
    #include "PluginAPI/IUnityGraphicsD3D9.h"
#endif
#ifdef fcSupportD3D11
    #include <d3d11.h>
    #include "PluginAPI/IUnityGraphicsD3D11.h"
#endif
#ifdef fcSupportD3D12
    #include <d3d12.h>
    #include "PluginAPI/IUnityGraphicsD3D12.h"
#endif

static IUnityInterfaces* g_unity_interface;

static void UNITY_INTERFACE_API UnityOnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    if (eventType == kUnityGfxDeviceEventInitialize) {
        auto unity_gfx = g_unity_interface->Get<IUnityGraphics>();
        auto api = unity_gfx->GetRenderer();

#ifdef fcSupportD3D11
        if (api == kUnityGfxRendererD3D11) {
            fcGfxInitializeD3D11(g_unity_interface->Get<IUnityGraphicsD3D11>()->GetDevice());
        }
#endif
#ifdef fcSupportD3D9
        if (api == kUnityGfxRendererD3D9) {
            fcGfxInitializeD3D9(g_unity_interface->Get<IUnityGraphicsD3D9>()->GetDevice());
        }
#endif
#ifdef fcSupportOpenGL
        if (api == kUnityGfxRendererOpenGL ||
            api == kUnityGfxRendererOpenGLCore ||
            api == kUnityGfxRendererOpenGLES20 ||
            api == kUnityGfxRendererOpenGLES30)
        {
            fcGfxInitializeOpenGL();
        }
#endif
    }
    else if (eventType == kUnityGfxDeviceEventShutdown) {
        fcGfxFinalize();
    }
}

fcCLinkage fcExport void fcCallDeferredCall(int id);
static void UNITY_INTERFACE_API UnityRenderEvent(int eventID)
{
    fcCallDeferredCall(eventID);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    g_unity_interface = unityInterfaces;
    g_unity_interface->Get<IUnityGraphics>()->RegisterDeviceEventCallback(UnityOnGraphicsDeviceEvent);
    UnityOnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    auto unity_gfx = g_unity_interface->Get<IUnityGraphics>();
    unity_gfx->UnregisterDeviceEventCallback(UnityOnGraphicsDeviceEvent);
}

fcCLinkage fcExport UnityRenderingEvent fcGetRenderEventFunc()
{
    return UnityRenderEvent;
}

fcCLinkage fcExport IUnityInterfaces* fcGetUnityInterface()
{
    return g_unity_interface;
}

#ifdef fcWindows
#include <windows.h>
typedef IUnityInterfaces* (*fcGetUnityInterfaceT)();

void fcGfxForceInitialize()
{
    // PatchLibrary で突っ込まれたモジュールは UnityPluginLoad() が呼ばれないので、
    // 先にロードされているモジュールからインターフェースをもらって同等の処理を行う。
    HMODULE m = ::GetModuleHandleA("FrameCapturer.dll");
    if (m) {
        auto proc = (fcGetUnityInterfaceT)::GetProcAddress(m, "fcGetUnityInterface");
        if (proc) {
            auto *iface = proc();
            if (iface) {
                UnityPluginLoad(iface);
            }
        }
    }
}
#endif

#endif // fcStaticLink
