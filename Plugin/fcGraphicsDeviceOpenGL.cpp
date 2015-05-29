#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"

#ifdef fcSupportOpenGL

#define GLEW_STATIC
#include <GL/glew.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32s.lib")


class fcGraphicsDeviceOpenGL : public fcGraphicsDevice
{
public:
    fcGraphicsDeviceOpenGL(void *device);
    ~fcGraphicsDeviceOpenGL();
    void* getDevicePtr() override;
    int getDeviceType() override;
    bool copyTextureData(void *o_buf, size_t bufsize, void *tex, int width, int height, fcETextureFormat format) override;

private:
    void *m_device;
};


fcGraphicsDevice* fcCreateGraphicsDeviceOpenGL(void *device)
{
    return new fcGraphicsDeviceOpenGL(device);
}


void* fcGraphicsDeviceOpenGL::getDevicePtr() { return m_device; }
int fcGraphicsDeviceOpenGL::getDeviceType() { return kGfxRendererOpenGL; }

fcGraphicsDeviceOpenGL::fcGraphicsDeviceOpenGL(void *device)
    : m_device(device)
{
    glewInit();
}

fcGraphicsDeviceOpenGL::~fcGraphicsDeviceOpenGL()
{
}


static void fcGetInternalFormatOpenGL(fcETextureFormat format, GLenum &o_fmt, GLenum &o_type)
{
    switch (format)
    {
    case fcE_ARGB32:    o_fmt = GL_RGBA; o_type = GL_UNSIGNED_BYTE; return;

    case fcE_ARGBHalf:  o_fmt = GL_RGBA; o_type = GL_HALF_FLOAT; return;
    case fcE_RGHalf:    o_fmt = GL_RG; o_type = GL_HALF_FLOAT; return;
    case fcE_RHalf:     o_fmt = GL_R; o_type = GL_HALF_FLOAT; return;

    case fcE_ARGBFloat: o_fmt = GL_RGBA; o_type = GL_FLOAT; return;
    case fcE_RGFloat:   o_fmt = GL_RG; o_type = GL_FLOAT; return;
    case fcE_RFloat:    o_fmt = GL_R; o_type = GL_FLOAT; return;

    case fcE_ARGBInt:   o_fmt = GL_RGBA; o_type = GL_INT; return;
    case fcE_RGInt:     o_fmt = GL_RG; o_type = GL_INT; return;
    case fcE_RInt:      o_fmt = GL_R; o_type = GL_INT; return;
    }
}

bool fcGraphicsDeviceOpenGL::copyTextureData(void *o_buf, size_t bufsize, void *tex, int width, int height, fcETextureFormat format)
{
    GLenum internal_format = 0;
    GLenum internal_type = 0;
    fcGetInternalFormatOpenGL(format, internal_format, internal_type);
    glGetTextureImage((GLuint)(size_t)tex, 0, internal_format, internal_type, bufsize, o_buf);
    return true;
}

#endif // fcSupportOpenGL
