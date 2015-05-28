#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"

#ifdef fcSupportOpenGL

// todo

class fcGraphicsDeviceOpenGL : public fcGraphicsDevice
{
public:
    fcGraphicsDeviceOpenGL(void *device);
    ~fcGraphicsDeviceOpenGL();
    bool copyTextureData(void *o_data, void *tex, int width, int height, int format) override;

    void* getDevicePtr() override;
    int getDeviceType() override;
private:
    void *m_device;
};


fcGraphicsDevice* fcCreateGraphicsDeviceOpenGL(void *device)
{
    return new fcGraphicsDeviceOpenGL(device);
}

fcGraphicsDeviceOpenGL::fcGraphicsDeviceOpenGL(void *device)
{
}

fcGraphicsDeviceOpenGL::~fcGraphicsDeviceOpenGL()
{
}

void* fcGraphicsDeviceOpenGL::getDevicePtr() { return m_device; }
int fcGraphicsDeviceOpenGL::getDeviceType() { return kGfxRendererOpenGL; }

bool fcGraphicsDeviceOpenGL::copyTextureData(void *o_data, void *tex_, int width, int height, int format)
{
    return false;
}

#endif // fcSupportOpenGL
