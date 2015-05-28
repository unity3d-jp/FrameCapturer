#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"

#ifdef fcSupportD3D11

class fcGraphicsDeviceD3D11 : public fcGraphicsDevice
{
public:
    fcGraphicsDeviceD3D11(void *device);
    ~fcGraphicsDeviceD3D11();
    void* createTmpTexture(int width, int height, fcETextureFormat format) override;
    void releaseTmpTexture(void *tex) override;
    bool copyTextureData(void *o_data, void *tex, void *tmp, int width, int height, int format) override;

    void* getDevicePtr() override;
    int getDeviceType() override;

private:
    ID3D11Device *m_device;
    ID3D11DeviceContext *m_context;

};


fcGraphicsDevice* fcCreateGraphicsDeviceD3D11(void *device)
{
    return new fcGraphicsDeviceD3D11(device);
}

fcGraphicsDeviceD3D11::fcGraphicsDeviceD3D11(void *device)
    : m_device((ID3D11Device*)device)
    , m_context(nullptr)
{
    if (m_device != nullptr)
    {
        m_device->GetImmediateContext(&m_context);
    }
}

fcGraphicsDeviceD3D11::~fcGraphicsDeviceD3D11()
{
    if (m_context != nullptr)
    {
        m_context->Release();
        m_context = nullptr;
    }
}

void* fcGraphicsDeviceD3D11::getDevicePtr() { return m_device; }
int fcGraphicsDeviceD3D11::getDeviceType() { return kGfxRendererD3D11; }

void* fcGraphicsDeviceD3D11::createTmpTexture(int width, int height, fcETextureFormat format)
{
    D3D11_TEXTURE2D_DESC desc = {
        width, height, 1, 1, DXGI_FORMAT_R8G8B8A8_TYPELESS, { 1, 0 },
        D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, 0
    };
    ID3D11Texture2D *ret = nullptr;
    HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &ret);
    return ret;
}

void fcGraphicsDeviceD3D11::releaseTmpTexture(void *tex)
{
    ((ID3D11Texture2D*)tex)->Release();
}

bool fcGraphicsDeviceD3D11::copyTextureData(void *o_data, void *tex_, void *tmp_, int width, int height, int format)
{
    if (m_context == nullptr || tex_ == nullptr || tmp_ == nullptr) { return false; }

    // Unity の RenderTexture は完全に CPU からはアクセス不可能になっているので、
    // staging texture を用意してそれに内容を移し、CPU はそれからデータを読む。
    ID3D11Texture2D *tex = (ID3D11Texture2D*)tex_;
    ID3D11Texture2D *tmp = (ID3D11Texture2D*)tmp_;
    m_context->CopyResource(tmp, tex);

    D3D11_MAPPED_SUBRESOURCE mapped = {nullptr, width*4, 0};
    HRESULT hr = m_context->Map(tmp, 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(o_data, mapped.pData, mapped.RowPitch * height);
        m_context->Unmap(tex, 0);
        return true;
    }
    return false;
}

#endif // fcSupportD3D11
