#ifndef FrameCapturer_h
#define FrameCapturer_h

//options:
//#define fcSupportGIF
//#define fcSupportEXR
//#define fcSupportMP4
//#define fcSupportOpenGL
//#define fcSupportD3D9
//#define fcSupportD3D11
//#define fcWithTBB

//#define fcGIFSplitModule
#define fcEXRSplitModule
#define fcMP4SplitModule


#if defined(_WIN32)
    #define fcWindows
#elif defined(__APPLE__)
    #ifdef TARGET_OS_IPHONE
        #define fciOS
    #else
        #define fcMac
    #endif
#elif defined(__ANDROID__)
    #define fcAndroid
#elif defined(__linux__)
    #define fcLinux
#endif

#ifdef _MSC_VER
    #define fcMSVC
#endif 


#define fcCLinkage extern "C"
#ifdef fcWindows
    #define fcExport __declspec(dllexport)
    #define fcBreak() DebugBreak()
#else // fcWindows
    #define fcExport
    #define fcBreak() __builtin_trap()
#endif // fcWindows

#ifdef fcDebug
    void fcDebugLogImpl(const char* fmt, ...);
    #define fcDebugLog(...) fcDebugLogImpl(__VA_ARGS__)
    #ifdef fcVerboseDebug
        #define fcDebugLogVerbose(...) fcDebugLogImpl(__VA_ARGS__)
    #else
        #define fcDebugLogVerbose(...)
    #endif
#else
    #define fcDebugLog(...)
    #define fcDebugLogVerbose(...)
#endif



class fcIGraphicsDevice;
class fcIExrContext;
class fcIGifContext;
class fcIMP4Context;

enum fcColorSpace
{
    fcColorSpace_RGBA,
    fcColorSpace_I420,
};

enum fcPixelFormat
{
    fcPixelFormat_Unknown,
    fcPixelFormat_RGBA8,
    fcPixelFormat_RGB8,
    fcPixelFormat_RG8,
    fcPixelFormat_R8,
    fcPixelFormat_RGBAHalf,
    fcPixelFormat_RGBHalf,
    fcPixelFormat_RGHalf,
    fcPixelFormat_RHalf,
    fcPixelFormat_RGBAFloat,
    fcPixelFormat_RGBFloat,
    fcPixelFormat_RGFloat,
    fcPixelFormat_RFloat,
    fcPixelFormat_RGBAInt,
    fcPixelFormat_RGBInt,
    fcPixelFormat_RGInt,
    fcPixelFormat_RInt,
};

enum fcTextureFormat
{
    fcTextureFormat_ARGB32 = 0,
    fcTextureFormat_Depth = 1,
    fcTextureFormat_ARGBHalf = 2,
    fcTextureFormat_Shadowmap = 3,
    fcTextureFormat_RGB565 = 4,
    fcTextureFormat_ARGB4444 = 5,
    fcTextureFormat_ARGB1555 = 6,
    fcTextureFormat_Default = 7,
    fcTextureFormat_ARGB2101010 = 8,
    fcTextureFormat_DefaultHDR = 9,
    fcTextureFormat_ARGBFloat = 11,
    fcTextureFormat_RGFloat = 12,
    fcTextureFormat_RGHalf = 13,
    fcTextureFormat_RFloat = 14,
    fcTextureFormat_RHalf = 15,
    fcTextureFormat_R8 = 16,
    fcTextureFormat_ARGBInt = 17,
    fcTextureFormat_RGInt = 18,
    fcTextureFormat_RInt = 19,
};


// -------------------------------------------------------------
// Foundation
// -------------------------------------------------------------

fcCLinkage fcExport void            fcSetModulePath(const char *path);
fcCLinkage fcExport const char*     fcGetModulePath(); // null-terminated path list. i.e. {"hoge", "hage", nullptr}

fcCLinkage fcExport uint64_t        fcMakeTimestamp();
fcCLinkage fcExport uint64_t        fcSecondsToTimestamp(double sec);


// -------------------------------------------------------------
// EXR Exporter
// -------------------------------------------------------------

struct fcExrConfig
{
    int max_active_tasks;
};
fcCLinkage fcExport fcIExrContext*  fcExrCreateContext(fcExrConfig *conf);
fcCLinkage fcExport void            fcExrDestroyContext(fcIExrContext *ctx);
fcCLinkage fcExport bool            fcExrBeginFrame(fcIExrContext *ctx, const char *path, int width, int height);
fcCLinkage fcExport bool            fcExrAddLayerTexture(fcIExrContext *ctx, void *tex, fcTextureFormat fmt, int ch, const char *name, bool flipY);
fcCLinkage fcExport bool            fcExrAddLayerPixels(fcIExrContext *ctx, const void *pixels, fcPixelFormat fmt, int ch, const char *name, bool flipY);
fcCLinkage fcExport bool            fcExrEndFrame(fcIExrContext *ctx);


// -------------------------------------------------------------
// GIF Exporter
// -------------------------------------------------------------

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


// -------------------------------------------------------------
// MP4 Exporter
// -------------------------------------------------------------

struct fcMP4Config
{
    int video;
    int audio;
    int video_width;
    int video_height;
    int video_bitrate;
    int video_framerate; // todo: this should be removed
    int video_max_buffers;
    int video_max_frame;
    int video_max_data_size;
    int audio_sample_rate;
    int audio_num_channels;
    int audio_bitrate;

    fcMP4Config()
        : video(true), audio(false)
        , video_width(320), video_height(240)
        , video_bitrate(256000), video_framerate(30)
        , video_max_buffers(8), video_max_frame(0), video_max_data_size(0)
        , audio_sample_rate(48000), audio_num_channels(2), audio_bitrate(64000)
    {}
};
typedef void(*fcDownloadCallback)(bool is_complete, const char *status);
fcCLinkage fcExport bool            fcMP4DownloadCodec(fcDownloadCallback cb);

fcCLinkage fcExport fcIMP4Context*  fcMP4CreateContext(fcMP4Config *conf);
fcCLinkage fcExport void            fcMP4DestroyContext(fcIMP4Context *ctx);
fcCLinkage fcExport bool            fcMP4AddVideoFrameTexture(fcIMP4Context *ctx, void *tex);
fcCLinkage fcExport bool            fcMP4AddVideoFramePixels(fcIMP4Context *ctx, void *pixels, fcColorSpace cs = fcColorSpace_RGBA);
fcCLinkage fcExport bool            fcMP4AddAudioSamples(fcIMP4Context *ctx, const float *samples, int num_samples);
fcCLinkage fcExport void            fcMP4ClearFrame(fcIMP4Context *ctx);
fcCLinkage fcExport bool            fcMP4WriteFile(fcIMP4Context *ctx, const char *path, int begin_frame, int end_frame);
fcCLinkage fcExport int             fcMP4WriteMemory(fcIMP4Context *ctx, void *buf, int begin_frame, int end_frame);
fcCLinkage fcExport int             fcMP4GetFrameCount(fcIMP4Context *ctx);
fcCLinkage fcExport void            fcMP4GetFrameData(fcIMP4Context *ctx, void *tex, int frame);
fcCLinkage fcExport int             fcMP4GetExpectedDataSize(fcIMP4Context *ctx, int begin_frame, int end_frame);
fcCLinkage fcExport void            fcMP4EraseFrame(fcIMP4Context *ctx, int begin_frame, int end_frame);

#endif // FrameCapturer_h
