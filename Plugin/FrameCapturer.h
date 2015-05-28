#ifndef FrameCapturer_h
#define FrameCapturer_h

class fcExrContext;
class fcGifContext;

enum fcEMagic
{
    fcE_ExtContext = 1,
    fcE_GifContext = 2,
};

enum fcETextureFormat
{
    fcE_ARGB32 = 0,
    fcE_Depth = 1,
    fcE_ARGBHalf = 2,
    fcE_Shadowmap = 3,
    fcE_RGB565 = 4,
    fcE_ARGB4444 = 5,
    fcE_ARGB1555 = 6,
    fcE_Default = 7,
    fcE_ARGB2101010 = 8,
    fcE_DefaultHDR = 9,
    fcE_ARGBFloat = 11,
    fcE_RGFloat = 12,
    fcE_RGHalf = 13,
    fcE_RFloat = 14,
    fcE_RHalf = 15,
    fcE_R8 = 16,
    fcE_ARGBInt = 17,
    fcE_RGInt = 18,
    fcE_RInt = 19,
};


struct fcExrConfig
{
    int width;
    int height;
    fcETextureFormat fmt;
};
fcCLinkage fcExport fcExrContext*   fcExrCreateFile(const char *path, fcExrConfig *conf);
fcCLinkage fcExport void            fcExrCloseFile(fcExrContext *ctx);
fcCLinkage fcExport void            fcExrWriteFrame(fcExrContext *ctx, void *tex);
fcCLinkage fcExport void            fcExrBeginWriteFrame(fcExrContext *ctx, void *tex);
fcCLinkage fcExport void            fcExrEndWriteFrame(fcExrContext *ctx);

struct fcGifConfig
{
    int width;
    int height;
    int interval;
    int keyframe;
    int max_active_tasks;
    int max_frame;
    int max_data_size;
};
fcCLinkage fcExport fcGifContext*   fcGifCreateFile(const char *path, fcGifConfig *conf);
fcCLinkage fcExport void            fcGifCloseFile(fcGifContext *ctx);
fcCLinkage fcExport void            fcGifWriteFrame(fcGifContext *ctx, void *tex);
fcCLinkage fcExport void            fcGifAsyncWriteFrame(fcGifContext *ctx, void *tex);

#endif // FrameCapturer_h
