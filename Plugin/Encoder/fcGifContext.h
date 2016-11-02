#ifndef fcGifFile_h
#define fcGifFile_h

class fcIGifContext
{
public:
    virtual void release() = 0;

    virtual bool addFrameTexture(void *tex, fcPixelFormat fmt, bool keyframe, fcTime timestamp = -1) = 0;
    virtual bool addFramePixels(const void *pixels, fcPixelFormat fmt, bool keyframe, fcTime timestamp = -1) = 0;
    virtual bool write(fcStream& stream, int begin_frame, int end_frame) = 0;

    virtual void clearFrame() = 0;
    virtual int  getFrameCount() = 0;
    virtual void getFrameData(void *tex, int frame) = 0;
    virtual int  getExpectedDataSize(int begin_frame, int end_frame) = 0;
    virtual void eraseFrame(int begin_frame, int end_frame) = 0;

protected:
    virtual ~fcIGifContext() {}
};
typedef fcIGifContext* (*fcGifCreateContextImplT)(const fcGifConfig &conf, fcIGraphicsDevice*);

#endif // fcGifFile_h
