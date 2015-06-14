#ifdef fcSupportMP4

class fcIMP4Context
{
public:
    virtual void release() = 0;

    virtual bool addFrameTexture(void *tex) = 0;
    virtual bool addFramePixels(void *pixels, fcEColorSpace cs)=0;
    virtual void clearFrame() = 0;
    virtual bool writeFile(const char *path, int begin_frame, int end_frame) = 0;
    virtual int  writeMemory(void *buf, int begin_frame, int end_frame) = 0;

    virtual int getFrameCount() = 0;
    virtual void getFrameData(void *tex, int frame) = 0;
    virtual int getExpectedDataSize(int begin_frame, int end_frame) = 0;
    virtual void eraseFrame(int begin_frame, int end_frame) = 0;

protected:
    virtual ~fcIMP4Context() {}
};

typedef fcIMP4Context* (*fcCreateMP4ContextT)(fcMP4Config &conf, fcIGraphicsDevice*);

#endif // fcSupportMP4
