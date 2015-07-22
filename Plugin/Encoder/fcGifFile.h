#ifdef fcSupportGIF

class fcIGifContext
{
public:
    virtual void release() = 0;

    virtual bool addFrame(void *tex) = 0;
    virtual void clearFrame() = 0;
    virtual bool writeFile(const char *path, int begin_frame, int end_frame) = 0;
    virtual int  writeMemory(void *buf, int begin_frame, int end_frame) = 0;

    virtual int getFrameCount() = 0;
    virtual void getFrameData(void *tex, int frame) = 0;
    virtual int getExpectedDataSize(int begin_frame, int end_frame) = 0;
    virtual void eraseFrame(int begin_frame, int end_frame) = 0;

protected:
    virtual ~fcIGifContext() {}
};
typedef fcIGifContext* (*fcGifCreateContextImplT)(fcGifConfig &conf, fcIGraphicsDevice*);

#endif // fcSupportGIF
