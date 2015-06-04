#ifdef fcSupportEXR

class fcIExrContext
{
public:
    virtual void release() = 0;
    virtual bool beginFrame(const char *path, int width, int height) = 0;
    virtual bool addLayer(void *tex, fcETextureFormat fmt, int channel, const char *name) = 0;
    virtual bool endFrame() = 0;
protected:
    virtual ~fcIExrContext() {}
};
typedef fcIExrContext* (*fcCreateExrContextT)(fcExrConfig &conf, fcIGraphicsDevice*);

#endif // fcSupportEXR
