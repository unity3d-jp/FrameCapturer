#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"

#ifdef fcSupportOpenGL

#define GLEW_STATIC
#include <GL/glew.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32s.lib")


class fcGraphicsDeviceOpenGL : public fcIGraphicsDevice
{
public:
    fcGraphicsDeviceOpenGL(void *device);
    ~fcGraphicsDeviceOpenGL();
    void* getDevicePtr() override;
    int getDeviceType() override;
    bool readTexture(void *o_buf, size_t bufsize, void *tex, int width, int height, fcETextureFormat format) override;
    bool writeTexture(void *o_tex, int width, int height, fcETextureFormat format, const void *buf, size_t bufsize) override;

private:
    void *m_device;
};


fcIGraphicsDevice* fcCreateGraphicsDeviceOpenGL(void *device)
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
    case fcE_RHalf:     o_fmt = GL_RED; o_type = GL_HALF_FLOAT; return;

    case fcE_ARGBFloat: o_fmt = GL_RGBA; o_type = GL_FLOAT; return;
    case fcE_RGFloat:   o_fmt = GL_RG; o_type = GL_FLOAT; return;
    case fcE_RFloat:    o_fmt = GL_RED; o_type = GL_FLOAT; return;

    case fcE_ARGBInt:   o_fmt = GL_RGBA_INTEGER; o_type = GL_INT; return;
    case fcE_RGInt:     o_fmt = GL_RG_INTEGER; o_type = GL_INT; return;
    case fcE_RInt:      o_fmt = GL_RED_INTEGER; o_type = GL_INT; return;
    }
}

bool fcGraphicsDeviceOpenGL::readTexture(void *o_buf, size_t bufsize, void *tex, int width, int height, fcETextureFormat format)
{
    GLenum internal_format = 0;
    GLenum internal_type = 0;
    fcGetInternalFormatOpenGL(format, internal_format, internal_type);

    //// glGetTextureImage() is available only OpenGL 4.5 or later...
    // glGetTextureImage((GLuint)(size_t)tex, 0, internal_format, internal_type, bufsize, o_buf);

    glFinish();
    glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)tex);
    glGetTexImage(GL_TEXTURE_2D, 0, internal_format, internal_type, o_buf);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool fcGraphicsDeviceOpenGL::writeTexture(void *o_tex, int width, int height, fcETextureFormat format, const void *buf, size_t bufsize)
{
    GLenum internal_format = 0;
    GLenum internal_type = 0;
    fcGetInternalFormatOpenGL(format, internal_format, internal_type);

    //// glTextureSubImage2D() is available only OpenGL 4.5 or later...
    // glTextureSubImage2D((GLuint)(size_t)o_tex, 0, 0, 0, width, height, internal_format, internal_type, buf);

    glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)o_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, internal_format, internal_type, buf);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

#endif // fcSupportOpenGL
