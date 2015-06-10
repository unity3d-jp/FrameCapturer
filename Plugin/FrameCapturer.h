#ifndef FrameCapturer_h
#define FrameCapturer_h

class fcIGraphicsDevice;
class fcIExrContext;
class fcIGifContext;
class fcIMP4Context;

enum fcEMagic
{
    fcE_Deleted    = 0xDEADBEEF,
    fcE_ExrContext = 0x10101010,
    fcE_GifContext = 0x20202020,
    fcE_MP4Context = 0x30303030,
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



#ifdef fcSupportEXR

struct fcExrConfig
{
    int max_active_tasks;
};
fcCLinkage fcExport fcIExrContext*  fcExrCreateContext(fcExrConfig *conf);
fcCLinkage fcExport void            fcExrDestroyContext(fcIExrContext *ctx);
fcCLinkage fcExport bool            fcExrBeginFrame(fcIExrContext *ctx, const char *path, int width, int height);
fcCLinkage fcExport bool            fcExrAddLayer(fcIExrContext *ctx, void *tex, fcETextureFormat fmt, int ch, const char *name);
fcCLinkage fcExport bool            fcExrEndFrame(fcIExrContext *ctx);

#endif // fcSupportEXR



#ifdef fcSupportGIF

struct fcGifConfig
{
    int width;
    int height;
    int num_colors;
    int delay_csec;
    int keyframe;
    int max_active_tasks;
    int max_frame;
    int max_data_size;
};
fcCLinkage fcExport fcIGifContext*  fcGifCreateContext(fcGifConfig *conf);
fcCLinkage fcExport void            fcGifDestroyContext(fcIGifContext *ctx);
fcCLinkage fcExport bool            fcGifAddFrame(fcIGifContext *ctx, void *tex);
fcCLinkage fcExport void            fcGifClearFrame(fcIGifContext *ctx);
fcCLinkage fcExport bool            fcGifWriteFile(fcIGifContext *ctx, const char *path, int begin_frame, int end_frame);
fcCLinkage fcExport int             fcGifWriteMemory(fcIGifContext *ctx, void *buf, int begin_frame, int end_frame);
fcCLinkage fcExport int             fcGifGetFrameCount(fcIGifContext *ctx);
fcCLinkage fcExport void            fcGifGetFrameData(fcIGifContext *ctx, void *tex, int frame);
fcCLinkage fcExport int             fcGifGetExpectedDataSize(fcIGifContext *ctx, int begin_frame, int end_frame);
fcCLinkage fcExport void            fcGifEraseFrame(fcIGifContext *ctx, int begin_frame, int end_frame);

#endif // fcSupportGIF



#ifdef fcSupportMP4

struct fcMP4Config
{
    int width;
    int height;
    int max_active_tasks;
    int max_frame;
    int max_data_size;
};
fcCLinkage fcExport fcIMP4Context*  fcMP4CreateContext(fcMP4Config *conf);
fcCLinkage fcExport void            fcMP4DestroyContext(fcIMP4Context *ctx);
fcCLinkage fcExport bool            fcMP4AddFrame(fcIMP4Context *ctx, void *tex);
fcCLinkage fcExport void            fcMP4ClearFrame(fcIMP4Context *ctx);
fcCLinkage fcExport bool            fcMP4WriteFile(fcIMP4Context *ctx, const char *path, int begin_frame, int end_frame);
fcCLinkage fcExport int             fcMP4WriteMemory(fcIMP4Context *ctx, void *buf, int begin_frame, int end_frame);
fcCLinkage fcExport int             fcMP4GetFrameCount(fcIMP4Context *ctx);
fcCLinkage fcExport void            fcMP4GetFrameData(fcIMP4Context *ctx, void *tex, int frame);
fcCLinkage fcExport int             fcMP4GetExpectedDataSize(fcIMP4Context *ctx, int begin_frame, int end_frame);
fcCLinkage fcExport void            fcMP4EraseFrame(fcIMP4Context *ctx, int begin_frame, int end_frame);

#endif // fcSupportMP4

#endif // FrameCapturer_h
