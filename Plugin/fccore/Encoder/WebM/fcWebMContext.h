#pragma once

class fcIWebMContext : public fcContextBase
{
public:
    virtual void addOutputStream(fcStream *s) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp = -1.0) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp = -1.0) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp = -1.0) = 0;
};

fcIWebMContext* fcWebMCreateContextImpl(fcWebMConfig &conf, fcIGraphicsDevice*);
