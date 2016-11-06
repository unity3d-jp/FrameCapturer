#pragma once

class fcIWebMContext
{
public:
    virtual void release() = 0;

    virtual void addOutputStream(fcStream *s) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp = -1.0) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp = -1.0) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp = -1.0) = 0;

protected:
    virtual ~fcIWebMContext() {}
};


#ifdef fcWebMSplitModule
    #define fcWebMAPI fcCLinkage fcExport
    typedef fcIWebMContext* (*fcWebMCreateContextImpl_t)(fcWebMConfig &conf, fcIGraphicsDevice*);
#else
    #define fcWebMAPI 
    fcWebMAPI fcIWebMContext*   fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice*);
#endif
