#pragma once

class fcIMP4Context : public fcContextBase
{
public:
    virtual bool isValid() const = 0;

    virtual const char* getVideoEncoderInfo() = 0;
    virtual const char* getAudioEncoderInfo() = 0;

    virtual void addOutputStream(fcStream *s) = 0;

    // assume texture format is RGBA8.
    // timestamp=-1 is treated as current time.
    virtual bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp = -1) = 0;

    // assume pixel format is RGBA8 or I420 (color_space indicates)
    // timestamp=-1 is treated as current time.
    virtual bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp = -1) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool AddAudioSamples(const float *samples, int num_samples) = 0;
};

#define fcMP4EachFunctions(Body)\
    Body(fcMP4SetModulePathImpl)\
    Body(fcMP4SetFAACPackagePathImpl)\
    Body(fcMP4DownloadCodecBeginImpl)\
    Body(fcMP4DownloadCodecGetStateImpl)\
    Body(fcMP4CreateContextImpl)\
    Body(fcMP4OSCreateContextImpl)

void            fcMP4SetModulePathImpl(const char *path);
fcIMP4Context*  fcMP4CreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice*);
bool            fcMP4OSIsSupportedImpl();
fcIMP4Context*  fcMP4OSCreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice *dev, const char *path);
