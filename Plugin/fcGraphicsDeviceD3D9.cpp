#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"

#ifdef fcSupportD3D9
#include <d3d9.h>
const int fcD3D9MaxStagingTextures = 32;

class fcGraphicsDeviceD3D9 : public fcGraphicsDevice
{
public:
    fcGraphicsDeviceD3D9(void *device);
    ~fcGraphicsDeviceD3D9();
    void* getDevicePtr() override;
    int getDeviceType() override;
    bool copyTextureData(void *o_buf, size_t bufsize, void *tex, int width, int height, fcETextureFormat format) override;

private:
    void clearStagingTextures();
    IDirect3DSurface9* findOrCreateStagingTexture(int width, int height, fcETextureFormat format);

private:
    IDirect3DDevice9 *m_device;
    std::map<uint64_t, IDirect3DSurface9*> m_staging_textures;
};


fcGraphicsDevice* fcCreateGraphicsDeviceD3D9(void *device)
{
    return new fcGraphicsDeviceD3D9(device);
}


fcGraphicsDeviceD3D9::fcGraphicsDeviceD3D9(void *device)
    : m_device((IDirect3DDevice9*)device)
{
}

fcGraphicsDeviceD3D9::~fcGraphicsDeviceD3D9()
{
    clearStagingTextures();
}

void* fcGraphicsDeviceD3D9::getDevicePtr() { return m_device; }
int fcGraphicsDeviceD3D9::getDeviceType() { return kGfxRendererD3D9; }


void fcGraphicsDeviceD3D9::clearStagingTextures()
{
    for (auto& pair : m_staging_textures)
    {
        pair.second->Release();
    }
    m_staging_textures.clear();
}



static D3DFORMAT fcGetInternalFormatD3D9(fcETextureFormat fmt)
{
    switch (fmt)
    {
    case fcE_ARGB32:    return D3DFMT_A8R8G8B8;

    case fcE_ARGBHalf:  return D3DFMT_A16B16G16R16F;
    case fcE_RGHalf:    return D3DFMT_G16R16F;
    case fcE_RHalf:     return D3DFMT_R16F;

    case fcE_ARGBFloat: return D3DFMT_A32B32G32R32F;
    case fcE_RGFloat:   return D3DFMT_G32R32F;
    case fcE_RFloat:    return D3DFMT_R32F;
    }
    return D3DFMT_UNKNOWN;
}

IDirect3DSurface9* fcGraphicsDeviceD3D9::findOrCreateStagingTexture(int width, int height, fcETextureFormat format)
{
    if (m_staging_textures.size() >= fcD3D9MaxStagingTextures) {
        clearStagingTextures();
    }

    D3DFORMAT internal_format = fcGetInternalFormatD3D9(format);
    if (internal_format == D3DFMT_UNKNOWN) { return nullptr; }

    uint64_t hash = width + (height << 16) + ((uint64_t)internal_format << 32);
    {
        auto it = m_staging_textures.find(hash);
        if (it != m_staging_textures.end())
        {
            return it->second;
        }
    }

    IDirect3DSurface9 *ret = nullptr;
    HRESULT hr = m_device->CreateOffscreenPlainSurface(width, height, internal_format, D3DPOOL_SYSTEMMEM, &ret, NULL);
    if (SUCCEEDED(hr))
    {
        m_staging_textures.insert(std::make_pair(hash, ret));
    }
    return ret;
}


template<class T>
struct RGBA
{
    T r,g,b,a;
};

template<class T>
inline void BGRA_To_RGBA(RGBA<T> *data, int data_num)
{
    for (int i = 0; i < data_num; ++i) {
        RGBA<T> &p = data[i];
        T t = p.b;
        p.b = p.r;
        p.r = t;
    }
}

bool fcGraphicsDeviceD3D9::copyTextureData(void *o_buf, size_t bufsize, void *tex_, int width, int height, fcETextureFormat format)
{
    HRESULT hr;
    IDirect3DTexture9 *tex = (IDirect3DTexture9*)tex_;

    // D3D11 と同様 render target の内容は CPU からはアクセス不可能になっている。
    // staging texture を用意してそれに内容を移し、CPU はそれ経由でデータを読む。
    IDirect3DSurface9 *surf_dst = findOrCreateStagingTexture(width, height, format);
    if (surf_dst == nullptr) { return false; }

    IDirect3DSurface9* surf_src = nullptr;
    hr = tex->GetSurfaceLevel(0, &surf_src);
    if (FAILED(hr)){ return false; }

    bool ret = false;
    hr = m_device->GetRenderTargetData(surf_src, surf_dst);
    if (SUCCEEDED(hr))
    {
        D3DLOCKED_RECT locked;
        hr = surf_dst->LockRect(&locked, nullptr, D3DLOCK_READONLY);
        if (SUCCEEDED(hr))
        {
            char *wpixels = (char*)o_buf;
            int wpitch = width * fcGetPixelSize(format);
            const char *rpixels = (const char*)locked.pBits;
            int rpitch = locked.Pitch;

            // D3D11 と同様表向き解像度と内部解像度が違うケースを考慮
            // (しかし、少なくとも手元の環境では常に wpitch == rpitch っぽい)
            if (wpitch == rpitch)
            {
                memcpy(wpixels, rpixels, bufsize);
            }
            else
            {
                for (int i = 0; i < height; ++i)
                {
                    memcpy(wpixels, rpixels, wpitch);
                    wpixels += wpitch;
                    rpixels += rpitch;
                }
            }

            surf_dst->UnlockRect();

            // D3D9 ではピクセルの並びは BGRA になっているので並べ替える
            if (format == fcE_ARGB32) { BGRA_To_RGBA((RGBA<uint8_t>*)o_buf, bufsize / 4); }
            ret = true;
        }
    }

    surf_src->Release();
    return ret;
}

#endif // fcSupportD3D9
