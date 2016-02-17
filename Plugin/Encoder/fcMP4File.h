#ifndef fcMP4File_h
#define fcMP4File_h

class fcIMP4Context
{
public:
    virtual void release() = 0;

    virtual void addOutputStream(fcStream *s) = 0;

    // assume texture format is RGBA8.
    // timestamp=-1 is treated as current time.
    virtual bool    addVideoFrameTexture(void *tex, fcTime timestamp = 0) = 0;

    // assume pixel format is RGBA8 or I420 (color_space indicates)
    // timestamp=-1 is treated as current time.
    virtual bool    addVideoFramePixels(const void *pixels, fcColorSpace color_space, fcTime timestamp = -1) = 0;

    // timestamp=-1 is treated as current time.
    virtual bool    addAudioFrame(const float *samples, int num_samples, fcTime timestamp = -1) = 0;

protected:
    virtual ~fcIMP4Context() {}
};

typedef void(*fcDownloadCallback)(bool is_complete, const char *status);

typedef void            (*fcMP4SetModulePathImplT)(const char *path);
typedef bool            (*fcMP4DownloadCodecImplT)(fcDownloadCallback cb);
typedef fcIMP4Context*  (*fcMP4CreateContextImplT)(fcMP4Config &conf, fcIGraphicsDevice*);

#endif // fcMP4File_h
