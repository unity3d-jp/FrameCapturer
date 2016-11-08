#ifdef _WIN32
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include "../FrameCapturer.h"


bool InitializeD3D11()
{
    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL valid_feature_level;

    IDXGIAdapter *adapter = nullptr;
    ID3D11Device *dev = nullptr;
    ID3D11DeviceContext *ctx = nullptr;
    HRESULT hr = D3D11CreateDevice(
        adapter,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        feature_levels,
        _countof(feature_levels),
        D3D11_SDK_VERSION,
        &dev,
        &valid_feature_level,
        &ctx);

    if (dev) {
        fcGfxInitializeD3D11(dev);
        return true;
    }
    else {
        return false;
    }
}

#else // _WIN32

bool InitializeD3D11() { return false; }

#endif // _WIN32
