#ifndef FrameCapturer_h
#define FrameCapturer_h

#define fcCLinkage extern "C"
#ifdef _WIN32
    #ifndef fcStaticLink
        #ifdef fcImpl
            #define fcExport __declspec(dllexport)
        #else
            #define fcExport __declspec(dllimport)
        #endif
    #else
        #define fcExport
    #endif
#else
    #define fcExport
#endif

#include <cstdint>

class fcIGraphicsDevice;
class fcIPngContext;
class fcIExrContext;
class fcIGifContext;
class fcIMP4Context;
class fcIWebMContext;
typedef double fcTime;

enum fcPixelFormat
{
    fcPixelFormat_Unknown = 0,

    fcPixelFormat_ChannelMask = 0xF,
    fcPixelFormat_TypeMask = 0xF << 4,
    fcPixelFormat_Type_f16 = 0x1 << 4,
    fcPixelFormat_Type_f32 = 0x2 << 4,
    fcPixelFormat_Type_u8  = 0x3 << 4,
    fcPixelFormat_Type_i16 = 0x4 << 4,
    fcPixelFormat_Type_i32 = 0x5 << 4,

    fcPixelFormat_Rf16      = fcPixelFormat_Type_f16 | 1,
    fcPixelFormat_RGf16     = fcPixelFormat_Type_f16 | 2,
    fcPixelFormat_RGBf16    = fcPixelFormat_Type_f16 | 3,
    fcPixelFormat_RGBAf16   = fcPixelFormat_Type_f16 | 4,
    fcPixelFormat_Rf32      = fcPixelFormat_Type_f32 | 1,
    fcPixelFormat_RGf32     = fcPixelFormat_Type_f32 | 2,
    fcPixelFormat_RGBf32    = fcPixelFormat_Type_f32 | 3,
    fcPixelFormat_RGBAf32   = fcPixelFormat_Type_f32 | 4,
    fcPixelFormat_Ru8       = fcPixelFormat_Type_u8  | 1,
    fcPixelFormat_RGu8      = fcPixelFormat_Type_u8  | 2,
    fcPixelFormat_RGBu8     = fcPixelFormat_Type_u8  | 3,
    fcPixelFormat_RGBAu8    = fcPixelFormat_Type_u8  | 4,
    fcPixelFormat_Ri16      = fcPixelFormat_Type_i16 | 1,
    fcPixelFormat_RGi16     = fcPixelFormat_Type_i16 | 2,
    fcPixelFormat_RGBi16    = fcPixelFormat_Type_i16 | 3,
    fcPixelFormat_RGBAi16   = fcPixelFormat_Type_i16 | 4,
    fcPixelFormat_Ri32      = fcPixelFormat_Type_i32 | 1,
    fcPixelFormat_RGi32     = fcPixelFormat_Type_i32 | 2,
    fcPixelFormat_RGBi32    = fcPixelFormat_Type_i32 | 3,
    fcPixelFormat_RGBAi32   = fcPixelFormat_Type_i32 | 4,
    fcPixelFormat_I420      = 0x10 << 4,
};


// -------------------------------------------------------------
// Foundation
// -------------------------------------------------------------

fcCLinkage fcExport void            fcGfxInitializeOpenGL();
fcCLinkage fcExport void            fcGfxInitializeD3D9(void *device);
fcCLinkage fcExport void            fcGfxInitializeD3D11(void *device);
fcCLinkage fcExport void            fcGfxFinalize();
fcCLinkage fcExport void            fcGfxSync();

fcCLinkage fcExport void            fcSetModulePath(const char *path);
fcCLinkage fcExport const char*     fcGetModulePath();
fcCLinkage fcExport fcTime          fcGetTime(); // current time in seconds


#ifndef fcImpl
struct fcStream;
#endif
// function types for custom stream
typedef size_t(*fcTellp_t)(void *obj);
typedef void(*fcSeekp_t)(void *obj, size_t pos);
typedef size_t(*fcWrite_t)(void *obj, const void *data, size_t len);

struct fcBufferData
{
    void *data = nullptr;
    size_t size = 0;
};
fcCLinkage fcExport fcStream*       fcCreateFileStream(const char *path);
fcCLinkage fcExport fcStream*       fcCreateMemoryStream();
fcCLinkage fcExport fcStream*       fcCreateCustomStream(void *obj, fcTellp_t tellp, fcSeekp_t seekp, fcWrite_t write);
fcCLinkage fcExport void            fcDestroyStream(fcStream *s);
fcCLinkage fcExport fcBufferData    fcStreamGetBufferData(fcStream *s); // s must be created by fcCreateMemoryStream(), otherwise return {nullptr, 0}.
fcCLinkage fcExport uint64_t        fcStreamGetWrittenSize(fcStream *s);


// -------------------------------------------------------------
// PNG Exporter
// -------------------------------------------------------------

struct fcPngConfig
{
    int max_active_tasks = 8;
};
fcCLinkage fcExport fcIPngContext*  fcPngCreateContext(const fcPngConfig *conf = nullptr);
fcCLinkage fcExport void            fcPngDestroyContext(fcIPngContext *ctx);
fcCLinkage fcExport bool            fcPngExportPixels(fcIPngContext *ctx, const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY = false);
fcCLinkage fcExport bool            fcPngExportTexture(fcIPngContext *ctx, const char *path, void *tex, int width, int height, fcPixelFormat fmt, bool flipY = false);


// -------------------------------------------------------------
// EXR Exporter
// -------------------------------------------------------------

struct fcExrConfig
{
    int max_active_tasks = 8;
};
fcCLinkage fcExport fcIExrContext*  fcExrCreateContext(const fcExrConfig *conf = nullptr);
fcCLinkage fcExport void            fcExrDestroyContext(fcIExrContext *ctx);
fcCLinkage fcExport bool            fcExrBeginFrame(fcIExrContext *ctx, const char *path, int width, int height);
fcCLinkage fcExport bool            fcExrAddLayerPixels(fcIExrContext *ctx, const void *pixels, fcPixelFormat fmt, int ch, const char *name, bool flipY = false);
fcCLinkage fcExport bool            fcExrAddLayerTexture(fcIExrContext *ctx, void *tex, fcPixelFormat fmt, int ch, const char *name, bool flipY = false);
fcCLinkage fcExport bool            fcExrEndFrame(fcIExrContext *ctx);


// -------------------------------------------------------------
// GIF Exporter
// -------------------------------------------------------------

struct fcGifConfig
{
    int width = 0;
    int height = 0;
    int num_colors = 256;
    int max_active_tasks = 8;
};
fcCLinkage fcExport fcIGifContext*  fcGifCreateContext(const fcGifConfig *conf);
fcCLinkage fcExport void            fcGifDestroyContext(fcIGifContext *ctx);
// timestamp=-1 is treated as current time.
fcCLinkage fcExport bool            fcGifAddFramePixels(fcIGifContext *ctx, const void *pixels, fcPixelFormat fmt, bool keyframe = false, fcTime timestamp = -1.0);
// timestamp=-1 is treated as current time.
fcCLinkage fcExport bool            fcGifAddFrameTexture(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, bool keyframe = false, fcTime timestamp = -1.0);
fcCLinkage fcExport bool            fcGifWrite(fcIGifContext *ctx, fcStream *stream, int begin_frame = 0, int end_frame = -1);

fcCLinkage fcExport void            fcGifClearFrame(fcIGifContext *ctx);
fcCLinkage fcExport int             fcGifGetFrameCount(fcIGifContext *ctx);
fcCLinkage fcExport void            fcGifGetFrameData(fcIGifContext *ctx, void *tex, int frame);
fcCLinkage fcExport int             fcGifGetExpectedDataSize(fcIGifContext *ctx, int begin_frame, int end_frame);
fcCLinkage fcExport void            fcGifEraseFrame(fcIGifContext *ctx, int begin_frame, int end_frame);


// -------------------------------------------------------------
// MP4 Exporter
// -------------------------------------------------------------

enum fcMP4VideoFlags
{
    fcMP4_H264NVIDIA    = 1 << 1,
    fcMP4_H264AMD       = 1 << 2,
    fcMP4_H264IntelHW   = 1 << 3,
    fcMP4_H264IntelSW   = 1 << 4,
    fcMP4_H264OpenH264  = 1 << 5,
    fcMP4_H264Mask = fcMP4_H264NVIDIA | fcMP4_H264AMD | fcMP4_H264IntelHW | fcMP4_H264IntelSW | fcMP4_H264OpenH264,
};

enum fcMP4AudioFlags
{
    fcMP4_AACIntel  = 1 << 1,
    fcMP4_AACFAAC   = 1 << 2,
    fcMP4_AACMask = fcMP4_AACIntel | fcMP4_AACFAAC,
};

struct fcMP4Config
{
    bool    video = true;
    bool    audio = true;

    int     video_width = 0;
    int     video_height = 0;
    int     video_target_bitrate = 1024 * 1000;
    int     video_target_framerate = 60;
    int     video_flags = fcMP4_H264Mask; // combination of fcMP4VideoFlags

    int     audio_sample_rate = 48000;
    int     audio_num_channels = 2;
    int     audio_target_bitrate = 128 * 1000;
    int     audio_flags = fcMP4_AACMask; // combination of fcMP4AudioFlags
};

enum fcDownloadState {
    fcDownloadState_Idle,
    fcDownloadState_Completed,
    fcDownloadState_Error,
    fcDownloadState_InProgress,
};
fcCLinkage fcExport void            fcMP4SetFAACPackagePath(const char *path);
fcCLinkage fcExport bool            fcMP4DownloadCodecBegin();
fcCLinkage fcExport fcDownloadState fcMP4DownloadCodecGetState();

fcCLinkage fcExport fcIMP4Context*  fcMP4CreateContext(fcMP4Config *conf);
fcCLinkage fcExport fcIMP4Context*  fcMP4CreateOSEncoderContext(fcMP4Config *conf, const char *out_path);
fcCLinkage fcExport void            fcMP4DestroyContext(fcIMP4Context *ctx);
fcCLinkage fcExport const char*     fcMP4GetAudioEncoderInfo(fcIMP4Context *ctx);
fcCLinkage fcExport const char*     fcMP4GetVideoEncoderInfo(fcIMP4Context *ctx);
fcCLinkage fcExport void            fcMP4AddOutputStream(fcIMP4Context *ctx, fcStream *stream);
// timestamp=-1 is treated as current time.
fcCLinkage fcExport bool            fcMP4AddVideoFramePixels(fcIMP4Context *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp = -1.0);
// timestamp=-1 is treated as current time.
fcCLinkage fcExport bool            fcMP4AddVideoFrameTexture(fcIMP4Context *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp = -1.0);
// timestamp=-1 is treated as current time.
fcCLinkage fcExport bool            fcMP4AddAudioFrame(fcIMP4Context *ctx, const float *samples, int num_samples, fcTime timestamp = -1.0);



// -------------------------------------------------------------
// WebM Exporter
// -------------------------------------------------------------

enum class fcWebMVideoEncoder
{
    VP8,
    VP9,
};
enum class fcWebMAudioEncoder
{
    Vorbis,
    Opus,
};

struct fcWebMConfig
{
    fcWebMVideoEncoder video_encoder = fcWebMVideoEncoder::VP8;
    fcWebMAudioEncoder audio_encoder = fcWebMAudioEncoder::Vorbis;
    bool    video = true;
    bool    audio = true;
    int     video_width = 0;
    int     video_height = 0;
    int     video_bitrate = 1024 * 1000;
    int     video_target_framerate = 60;
    int     video_max_buffers = 8;
    int     audio_sample_rate = 48000;
    int     audio_num_channels = 2;
    int     audio_bitrate = 64000;
};

fcCLinkage fcExport fcIWebMContext* fcWebMCreateContext(fcWebMConfig *conf);
fcCLinkage fcExport void            fcWebMDestroyContext(fcIWebMContext *ctx);
fcCLinkage fcExport void            fcWebMAddOutputStream(fcIWebMContext *ctx, fcStream *stream);
// timestamp=-1 is treated as current time.
fcCLinkage fcExport bool            fcWebMAddVideoFramePixels(fcIWebMContext *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp = -1.0);
// timestamp=-1 is treated as current time.
fcCLinkage fcExport bool            fcWebMAddVideoFrameTexture(fcIWebMContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp = -1.0);
// timestamp=-1 is treated as current time.
fcCLinkage fcExport bool            fcWebMAddAudioFrame(fcIWebMContext *ctx, const float *samples, int num_samples, fcTime timestamp = -1.0);

#endif // FrameCapturer_h
