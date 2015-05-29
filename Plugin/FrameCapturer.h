#ifndef FrameCapturer_h
#define FrameCapturer_h

class fcExrContext;
class fcGifContext;

enum fcEMagic
{
    fcE_ExrContext = 1,
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
    int max_active_tasks;
};
fcCLinkage fcExport fcExrContext*   fcExrCreateContext(fcExrConfig *conf);
fcCLinkage fcExport void            fcExrDestroyContext(fcExrContext *ctx);
fcCLinkage fcExport void            fcExrWriteFile(fcExrContext *ctx, void *tex, fcETextureFormat fmt, const char *path);


struct fcGifConfig
{
    int width;
    int height;
    int delay_csec;
    int keyframe;
    int max_active_tasks;
    int max_frame;
    int max_data_size;
};
fcCLinkage fcExport fcGifContext*   fcGifCreateContext(fcGifConfig *conf);
fcCLinkage fcExport void            fcGifDestroyContext(fcGifContext *ctx);
fcCLinkage fcExport void            fcGifAddFrame(fcGifContext *ctx, void *tex);
fcCLinkage fcExport void            fcGifClearFrame(fcGifContext *ctx);
fcCLinkage fcExport bool            fcGifWriteFile(fcGifContext *ctx, const char *path);

#endif // FrameCapturer_h
