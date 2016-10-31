#pragma once

class fcIMP4Context
{
public:
    virtual void release() = 0;

    virtual const char* getAudioEncoderInfo() = 0;
    virtual const char* getVideoEncoderInfo() = 0;

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
    virtual ~fcIMP4Context() {}
};

#define fcMP4EachFunctions(Body)\
    Body(fcMP4SetModulePathImpl)\
    Body(fcMP4SetFAACPackagePathImpl)\
    Body(fcMP4DownloadCodecBeginImpl)\
    Body(fcMP4DownloadCodecGetStateImpl)\
    Body(fcMP4CreateContextImpl)

#ifdef fcMP4SplitModule
    #define fcMP4API fcCLinkage fcExport
    typedef void            (*fcMP4SetModulePathImpl_t)(const char *path);
    typedef void            (*fcMP4SetFAACPackagePathImpl_t)(const char *path);
    typedef bool            (*fcMP4DownloadCodecBeginImpl_t)();
    typedef fcDownloadState (*fcMP4DownloadCodecGetStateImpl_t)();
    typedef fcIMP4Context*  (*fcMP4CreateContextImpl_t)(fcMP4Config &conf, fcIGraphicsDevice*);
#else
    #define fcMP4API 
    fcMP4API void            fcMP4SetModulePathImpl(const char *path);
    fcMP4API void            fcMP4SetFAACPackagePathImpl(const char *path);
    fcMP4API bool            fcMP4DownloadCodecBeginImpl();
    fcMP4API fcDownloadState fcMP4DownloadCodecGetStateImpl();
    fcMP4API fcIMP4Context*  fcMP4CreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice*);
#endif
