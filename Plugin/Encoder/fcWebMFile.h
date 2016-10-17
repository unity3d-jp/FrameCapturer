#pragma once


struct fcWebMConfig
{
};


class fcIWebMContext
{
public:
    virtual void release() = 0;

    virtual void addOutputStream(fcStream *s) = 0;

    // assume texture format is RGBA8.
    // timestamp=-1 is treated as current time.
    virtual bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp = -1) = 0;

    // assume pixel format is RGBA8 or I420 (color_space indicates)
    // timestamp=-1 is treated as current time.
    virtual bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp = -1) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp = -1) = 0;

protected:
    virtual ~fcIWebMContext();
};


#define fcWebMEachFunctions(Body)\
    Body(fcWebMSetModulePathImpl)\
    Body(fcWebMCreateContextImpl)

#ifdef fcWebMSplitModule
    #define fcWebMAPI fcCLinkage fcExport
    typedef void            (*fcWebMSetModulePathImpl_t)(const char *path);
    typedef fcIWebMContext* (*fcWebMCreateContextImpl_t)(fcWebMConfig &conf, fcIGraphicsDevice*);
#else
    #define fcWebMAPI 
    fcWebMAPI void              fcWebMSetModulePathImpl(const char *path);
    fcWebMAPI fcIWebMContext*   fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice*);
#endif
