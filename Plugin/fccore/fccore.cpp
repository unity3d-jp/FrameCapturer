#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"

//#define fcTraceFunc(...)  DebugLogImpl(__FUNCTION__ "\n")
#define fcTraceFunc(...)



// -------------------------------------------------------------
// Foundation
// -------------------------------------------------------------

namespace {
    std::string g_fcModulePath;
}

fcAPI void fcSetModulePath(const char *path)
{
    fcTraceFunc();
    g_fcModulePath = path;
    DLLAddSearchPath(path);
}

fcAPI const char* fcGetModulePath()
{
    fcTraceFunc();
    return !g_fcModulePath.empty() ? g_fcModulePath.c_str() : DLLGetDirectoryOfCurrentModule();
}

fcAPI fcTime fcGetTime()
{
    return GetCurrentTimeInSeconds();
}

fcAPI fcStream* fcCreateFileStream(const char *path)
{
    fcTraceFunc();
    return new StdIOStream(new std::fstream(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc), true);
}
fcAPI fcStream* fcCreateMemoryStream()
{
    fcTraceFunc();
    return new BufferStream(new Buffer(), true);
}
fcAPI fcStream* fcCreateCustomStream(void *obj, fcTellp_t tellp, fcSeekp_t seekp, fcWrite_t write)
{
    fcTraceFunc();
    CustomStreamData csd;
    csd.obj = obj;
    csd.tellp = tellp;
    csd.seekp = seekp;
    csd.write = write;
    return new CustomStream(csd);
}

fcAPI void fcDestroyStream(fcStream *s)
{
    fcTraceFunc();
    delete s;
}

fcAPI fcBufferData fcStreamGetBufferData(fcStream *s)
{
    fcTraceFunc();
    fcBufferData ret;
    if (BufferStream *bs = dynamic_cast<BufferStream*>(s)) {
        ret.data = bs->get().data();
        ret.size = bs->get().size();
    }
    return ret;
}

fcAPI uint64_t fcStreamGetWrittenSize(fcStream *s)
{
    fcTraceFunc();
    return s->tellp();
}


// -------------------------------------------------------------
// deferred call
// -------------------------------------------------------------

using fcDeferredCall = std::function<void()>;
namespace {
    std::mutex g_deferred_calls_mutex;
    std::vector<fcDeferredCall> g_deferred_calls;
}

fcAPI void fcGuardBegin()
{
    fcTraceFunc();
    g_deferred_calls_mutex.lock();
}

fcAPI void fcGuardEnd()
{
    fcTraceFunc();
    g_deferred_calls_mutex.unlock();
}

fcAPI int fcAllocateDeferredCall()
{
    fcTraceFunc();

    // search empty object and return its position if found
    for (int i = 1; i < (int)g_deferred_calls.size(); ++i) {
        if (!g_deferred_calls[i]) {
            g_deferred_calls[i] = fcDeferredCall();
            return i;
        }
    }

    // 0th is "null" object
    if (g_deferred_calls.empty()) { g_deferred_calls.emplace_back(fcDeferredCall()); }

    // allocate new one
    g_deferred_calls.emplace_back(fcDeferredCall());
    return (int)g_deferred_calls.size() - 1;
}

fcAPI int fcAddDeferredCall(const fcDeferredCall& dc, int id)
{
    fcTraceFunc();
    if (id <= 0) {
        // search empty object and return its position if found
        for (int i = 1; i < (int)g_deferred_calls.size(); ++i) {
            if (!g_deferred_calls[i]) {
                g_deferred_calls[i] = dc;
                return i;
            }
        }

        // 0th is "null" object
        if (g_deferred_calls.empty()) { g_deferred_calls.emplace_back(fcDeferredCall()); }

        // allocate new one
        g_deferred_calls.emplace_back(dc);
        return (int)g_deferred_calls.size() - 1;
    }
    else if(id < (int)g_deferred_calls.size()) {
        g_deferred_calls[id] = dc;
        return id;
    }
    else {
        fcDebugLog("fcAddDeferredCall(): should not be here");
        return 0;
    }
}

fcAPI void fcReleaseDeferredCall(int id)
{
    fcTraceFunc();
    if (id <= 0 || id >= (int)g_deferred_calls.size()) { return; }

    g_deferred_calls[id] = fcDeferredCall();
}

// **called from rendering thread**
fcAPI void fcCallDeferredCall(int id)
{
    fcTraceFunc();
    std::unique_lock<std::mutex> l(g_deferred_calls_mutex);
    if (id <= 0 || id >= (int)g_deferred_calls.size()) { return; }

    auto& dc = g_deferred_calls[id];
    if (dc) { dc(); }
}



// -------------------------------------------------------------
// PNG Exporter
// -------------------------------------------------------------

#ifdef fcSupportPNG
#include "Encoder/Image/fcPngContext.h"

fcAPI bool fcPngIsSupported() { return true; }

fcAPI fcIPngContext* fcPngCreateContext(const fcPngConfig *conf)
{
    fcTraceFunc();
    return fcPngCreateContextImpl(conf, fcGetGraphicsDevice());
}

fcAPI void fcPngDestroyContext(fcIPngContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->release();
}

fcAPI bool fcPngExportPixels(fcIPngContext *ctx, const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, int num_channels)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->exportPixels(path, pixels, width, height, fmt, num_channels);
}

fcAPI bool fcPngExportTexture(fcIPngContext *ctx, const char *path, void *tex, int width, int height, fcPixelFormat fmt, int num_channels)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->exportTexture(path, tex, width, height, fmt, num_channels);
}

fcAPI int fcPngExportTextureDeferred(fcIPngContext *ctx, const char *path_, void *tex, int width, int height, fcPixelFormat fmt, int num_channels, int id)
{
    fcTraceFunc();
    if (!ctx) { return 0; }

    std::string path = path_;
    return fcAddDeferredCall([=]() {
        ctx->exportTexture(path.c_str(), tex, width, height, fmt, num_channels);
    }, id);
}

#else // fcSupportPNG

fcAPI bool fcPngIsSupported() { return false; }
fcAPI fcIPngContext* fcPngCreateContext(const fcPngConfig *conf) { return nullptr; }
fcAPI void fcPngDestroyContext(fcIPngContext *ctx) { return; }
fcAPI bool fcPngExportPixels(fcIPngContext *ctx, const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, int num_channels) { return false; }
fcAPI bool fcPngExportTexture(fcIPngContext *ctx, const char *path, void *tex, int width, int height, fcPixelFormat fmt, int num_channels) { return false; }
fcAPI int fcPngExportTextureDeferred(fcIPngContext *ctx, const char *path_, void *tex, int width, int height, fcPixelFormat fmt, int num_channels, int id) { return 0; }

#endif // fcSupportPNG


// -------------------------------------------------------------
// EXR Exporter
// -------------------------------------------------------------

#ifdef fcSupportEXR
#include "Encoder/Image/fcExrContext.h"

fcAPI bool fcExrIsSupported() { return true; }

fcAPI fcIExrContext* fcExrCreateContext(const fcExrConfig *conf)
{
    fcTraceFunc();
    return fcExrCreateContextImpl(conf, fcGetGraphicsDevice());
}

fcAPI void fcExrDestroyContext(fcIExrContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->release();
}

fcAPI bool fcExrBeginImage(fcIExrContext *ctx, const char *path, int width, int height)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->beginFrame(path, width, height);
}

fcAPI bool fcExrAddLayerPixels(fcIExrContext *ctx, const void *pixels, fcPixelFormat fmt, int ch, const char *name)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addLayerPixels(pixels, fmt, ch, name);
}

fcAPI bool fcExrAddLayerTexture(fcIExrContext *ctx, void *tex, fcPixelFormat fmt, int ch, const char *name)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addLayerTexture(tex, fmt, ch, name);
}

fcAPI bool fcExrEndImage(fcIExrContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->endFrame();
}

fcAPI int fcExrBeginImageDeferred(fcIExrContext *ctx, const char *path_, int width, int height, int id)
{
    fcTraceFunc();
    if (!ctx) { return 0; }
    std::string path = path_; // hold to deferred call
    return fcAddDeferredCall([=]() {
        return ctx->beginFrame(path.c_str(), width, height);
    }, id);
}

fcAPI int fcExrAddLayerTextureDeferred(fcIExrContext *ctx, void *tex, fcPixelFormat fmt, int ch, const char *name_, int id)
{
    fcTraceFunc();
    if (!ctx) { return 0; }
    std::string name = name_;
    return fcAddDeferredCall([=]() {
        return ctx->addLayerTexture(tex, fmt, ch, name.c_str());
    }, id);
}

fcAPI int fcExrEndImageDeferred(fcIExrContext *ctx, int id)
{
    fcTraceFunc();
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->endFrame();
    }, id);
}

#else // fcSupportEXR

fcAPI bool fcExrIsSupported() { return false; }
fcAPI fcIExrContext* fcExrCreateContext(const fcExrConfig *conf) {}
fcAPI void fcExrDestroyContext(fcIExrContext *ctx) {}
fcAPI bool fcExrBeginImage(fcIExrContext *ctx, const char *path, int width, int height) { return false; }
fcAPI bool fcExrAddLayerPixels(fcIExrContext *ctx, const void *pixels, fcPixelFormat fmt, int ch, const char *name) { return false; }
fcAPI bool fcExrAddLayerTexture(fcIExrContext *ctx, void *tex, fcPixelFormat fmt, int ch, const char *name) { return false; }
fcAPI bool fcExrEndImage(fcIExrContext *ctx) { return false; }
fcAPI int fcExrBeginImageDeferred(fcIExrContext *ctx, const char *path_, int width, int height, int id) { return 0; }
fcAPI int fcExrAddLayerTextureDeferred(fcIExrContext *ctx, void *tex, fcPixelFormat fmt, int ch, const char *name_, int id) { return 0; }
fcAPI int fcExrEndImageDeferred(fcIExrContext *ctx, int id) { return 0; }

#endif // fcSupportEXR


// -------------------------------------------------------------
// GIF Exporter
// -------------------------------------------------------------

#ifdef fcSupportGIF
#include "Encoder/Image/fcGifContext.h"

fcAPI bool fcGifIsSupported() { return true; }

fcAPI fcIGifContext* fcGifCreateContext(const fcGifConfig *conf)
{
    fcTraceFunc();
    return fcGifCreateContextImpl(*conf, fcGetGraphicsDevice());
}

fcAPI void fcGifDestroyContext(fcIGifContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->release();
}

fcAPI void fcGifAddOutputStream(fcIGifContext *ctx, fcStream *stream)
{
    fcTraceFunc();
    if (!ctx) { return; }
    return ctx->addOutputStream(stream);
}

fcAPI bool fcGifAddFramePixels(fcIGifContext *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addFramePixels(pixels, fmt, timestamp);
}
fcAPI bool fcGifAddFrameTexture(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addFrameTexture(tex, fmt, timestamp);
}
fcAPI int fcGifAddFrameTextureDeferred(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp, int id)
{
    fcTraceFunc();
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->addFrameTexture(tex, fmt, timestamp);
    }, id);
}
fcAPI void fcGifForceKeyframe(fcIGifContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->forceKeyframe();
}

#else // fcSupportGIF

fcAPI bool fcGifIsSupported() { return false; }
fcAPI fcIGifContext* fcGifCreateContext(const fcGifConfig *conf) { return nullptr; }
fcAPI void fcGifDestroyContext(fcIGifContext *ctx) {}
fcAPI void fcGifAddOutputStream(fcIGifContext *ctx, fcStream *stream) {}
fcAPI bool fcGifAddFramePixels(fcIGifContext *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp) { return false; }
fcAPI bool fcGifAddFrameTexture(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp) { return false; }
fcAPI int fcGifAddFrameTextureDeferred(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp, int id) { return 0; }
fcAPI void fcGifForceKeyframe(fcIGifContext *ctx) {}

#endif // fcSupportGIF



// -------------------------------------------------------------
// MP4 Exporter
// -------------------------------------------------------------

#ifdef fcSupportMP4
#include "Encoder/MP4/fcMP4Context.h"

fcAPI bool fcMP4IsSupported() { return true; }
fcAPI bool fcMP4OSIsSupported() { return fcMP4OSIsSupportedImpl(); }

fcAPI fcIMP4Context* fcMP4CreateContext(fcMP4Config *conf)
{
    fcTraceFunc();
    return fcMP4CreateContextImpl(*conf, fcGetGraphicsDevice());
}

fcAPI fcIMP4Context* fcMP4OSCreateContext(fcMP4Config *conf, const char *out_path)
{
    fcTraceFunc();
    return fcMP4OSCreateContextImpl(*conf, fcGetGraphicsDevice(), out_path);
}

fcAPI void fcMP4DestroyContext(fcIMP4Context *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->release();
}

fcAPI const char* fcMP4GetVideoEncoderInfo(fcIMP4Context *ctx)
{
    fcTraceFunc();
    if (!ctx) { return ""; }
    return ctx->getVideoEncoderInfo();
}
fcAPI const char* fcMP4GetAudioEncoderInfo(fcIMP4Context *ctx)
{
    fcTraceFunc();
    if (!ctx) { return ""; }
    return ctx->getAudioEncoderInfo();
}

fcAPI void fcMP4AddOutputStream(fcIMP4Context *ctx, fcStream *stream)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->addOutputStream(stream);
}

fcAPI bool fcMP4AddVideoFramePixels(fcIMP4Context *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addVideoFramePixels(pixels, fmt, timestamp);
}
fcAPI bool fcMP4AddVideoFrameTexture(fcIMP4Context *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addVideoFrameTexture(tex, fmt, timestamp);
}
fcAPI int fcMP4AddVideoFrameTextureDeferred(fcIMP4Context *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp, int id)
{
    fcTraceFunc();
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->addVideoFrameTexture(tex, fmt, timestamp);
    }, id);
}

fcAPI bool fcMP4AddAudioFrame(fcIMP4Context *ctx, const float *samples, int num_samples, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addAudioFrame(samples, num_samples, timestamp);
}
#else // fcSupportMP4

fcAPI bool fcMP4IsSupported() { return false; }
fcAPI bool fcMP4OSIsSupported() { return false; }
fcAPI fcIMP4Context* fcMP4CreateContext(fcMP4Config *conf) { return nullptr; }
fcAPI fcIMP4Context* fcMP4OSCreateContext(fcMP4Config *conf, const char *out_path) { return nullptr; }
fcAPI void fcMP4DestroyContext(fcIMP4Context *ctx) {}
fcAPI const char* fcMP4GetVideoEncoderInfo(fcIMP4Context *ctx) { return ""; }
fcAPI const char* fcMP4GetAudioEncoderInfo(fcIMP4Context *ctx) { return ""; }
fcAPI void fcMP4AddOutputStream(fcIMP4Context *ctx, fcStream *stream) {}
fcAPI bool fcMP4AddVideoFramePixels(fcIMP4Context *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp) { return false; }
fcAPI bool fcMP4AddVideoFrameTexture(fcIMP4Context *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp) { return false; }
fcAPI int fcMP4AddVideoFrameTextureDeferred(fcIMP4Context *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp, int id) { return 0; }
fcAPI bool fcMP4AddAudioFrame(fcIMP4Context *ctx, const float *samples, int num_samples, fcTime timestamp) { return false; }

#endif // fcSupportMP4



// -------------------------------------------------------------
// WebM Exporter
// -------------------------------------------------------------

#ifdef fcSupportWebM
#include "Encoder/WebM/fcWebMContext.h"

fcAPI bool fcWebMIsSupported() { return true; }

fcAPI fcIWebMContext* fcWebMCreateContext(fcWebMConfig *conf)
{
    fcTraceFunc();
    return fcWebMCreateContextImpl(*conf, fcGetGraphicsDevice());
}

fcAPI void fcWebMDestroyContext(fcIWebMContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->release();
}

fcAPI void fcWebMAddOutputStream(fcIWebMContext *ctx, fcStream *stream)
{
    if (!ctx) { return; }
    ctx->addOutputStream(stream);
}

fcAPI bool fcWebMAddVideoFramePixels(fcIWebMContext *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addVideoFramePixels(pixels, fmt, timestamp);
}

fcAPI bool fcWebMAddVideoFrameTexture(fcIWebMContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addVideoFrameTexture(tex, fmt, timestamp);
}

fcAPI int fcWebMAddVideoFrameTextureDeferred(fcIWebMContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp, int id)
{
    fcTraceFunc();
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->addVideoFrameTexture(tex, fmt, timestamp);
    }, id);
}

fcAPI bool fcWebMAddAudioFrame(fcIWebMContext *ctx, const float *samples, int num_samples, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addAudioFrame(samples, num_samples, timestamp);
}

#else // fcSupportWebM

fcAPI bool fcWebMIsSupported() { return false; }
fcAPI fcIWebMContext* fcWebMCreateContext(fcWebMConfig *conf) { return nullptr; }
fcAPI void fcWebMDestroyContext(fcIWebMContext *ctx) {}
fcAPI void fcWebMAddOutputStream(fcIWebMContext *ctx, fcStream *stream) {}
fcAPI bool fcWebMAddVideoFramePixels(fcIWebMContext *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp) { return false; }
fcAPI bool fcWebMAddVideoFrameTexture(fcIWebMContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp) { return false; }
fcAPI int fcWebMAddVideoFrameTextureDeferred(fcIWebMContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp, int id) { return 0; }
fcAPI bool fcWebMAddAudioFrame(fcIWebMContext *ctx, const float *samples, int num_samples, fcTime timestamp) { return false; }

#endif // fcSupportWebM


#ifdef fcSupportWave
#include "Encoder/Audio/fcWaveContext.h"

fcAPI bool fcWaveIsSupported() { return true; }

fcAPI fcIWaveContext* fcWaveCreateContext(fcWaveConfig *conf)
{
    fcTraceFunc();
    if (!conf) { return nullptr; }
    return fcWaveCreateContextImpl(conf);
}

fcAPI void fcWaveDestroyContext(fcIWaveContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->release();
}

fcAPI void fcWaveAddOutputStream(fcIWaveContext *ctx, fcStream *stream)
{
    fcTraceFunc();
    if (!ctx || !stream) { return; }
    ctx->addOutputStream(stream);
}

fcAPI bool fcWaveAddAudioFrame(fcIWaveContext *ctx, const float *samples, int num_samples)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->write(samples, num_samples);
}

#else // fcSupportWave

fcAPI bool            fcWaveIsSupported() { return false; }
fcAPI fcIWaveContext* fcWaveCreateContext(fcWaveConfig *conf) { return nullptr; }
fcAPI void            fcWaveDestroyContext(fcIWaveContext *ctx) {}
fcAPI void            fcWaveAddOutputStream(fcIWaveContext *ctx, fcStream *stream) {}
fcAPI bool            fcWaveAddAudioFrame(fcIWaveContext *ctx, const float *samples, int num_samples) { return false; }

#endif // fcSupportWave


#ifdef fcSupportVorbis
#include "Encoder/Audio/fcOggContext.h"

fcAPI bool fcOggIsSupported() { return true; }

fcAPI fcIOggContext*  fcOggCreateContext(fcOggConfig *conf)
{
    fcTraceFunc();
    if (!conf) { return nullptr; }
    return fcOggCreateContextImpl(conf);
}

fcAPI void fcOggDestroyContext(fcIOggContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->release();
}

fcAPI void fcOggAddOutputStream(fcIOggContext *ctx, fcStream *stream)
{
    fcTraceFunc();
    if (!ctx || !stream) { return; }
    ctx->addOutputStream(stream);
}

fcAPI bool fcOggAddAudioFrame(fcIOggContext *ctx, const float *samples, int num_samples)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->write(samples, num_samples);
}

#else // fcSupportVorbis

fcAPI bool            fcOggIsSupported() { return false; }
fcAPI fcIOggContext*  fcOggCreateContext(fcOggConfig *conf) { return nullptr; }
fcAPI void            fcOggDestroyContext(fcIOggContext *ctx) {}
fcAPI void            fcOggAddOutputStream(fcIOggContext *ctx, fcStream *stream) {}
fcAPI bool            fcOggAddAudioFrame(fcIOggContext *ctx, const float *samples, int num_samples) { return false; }

#endif // fcSupportVorbis


// -------------------------------------------------------------
// Flac Exporter
// -------------------------------------------------------------

#ifdef fcSupportFlac
#include "Encoder/Audio/fcFlacContext.h"

fcAPI bool fcFlacIsSupported() { return true; }

fcAPI fcIFlacContext* fcFlacCreateContext(fcFlacConfig *conf)
{
    fcTraceFunc();
    if (!conf) { return nullptr; }
    return fcFlacCreateContextImpl(conf);
}

fcAPI void fcFlacDestroyContext(fcIFlacContext *ctx)
{
    fcTraceFunc();
    if (!ctx) { return; }
    ctx->release();
}

fcAPI void fcFlacAddOutputStream(fcIFlacContext *ctx, fcStream *stream)
{
    fcTraceFunc();
    if (!ctx || !stream) { return; }
    ctx->addOutputStream(stream);
}

fcAPI bool fcFlacAddAudioFrame(fcIFlacContext *ctx, const float *samples, int num_samples)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->write(samples, num_samples);
}

#else // fcSupportFlac

fcAPI bool            fcFlacIsSupported() { return false; }
fcAPI fcIFlacContext* fcFlacCreateContext(fcFlacConfig *conf) { return nullptr; }
fcAPI void            fcFlacDestroyContext(fcIFlacContext *ctx) { return; }
fcAPI void            fcFlacAddOutputStream(fcIFlacContext *ctx, fcStream *stream) { return; }
fcAPI bool            fcFlacAddAudioFrame(fcIFlacContext *ctx, const float *samples, int num_samples) { return false; }

#endif // fcSupportFlac





#if defined(fcWindows) && !defined(fcStaticLink)
#include <windows.h>

void fcGfxForceInitialize();

BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call == DLL_PROCESS_ATTACH)
    {
        // initialize graphics device
        fcGfxForceInitialize();
    }
    return TRUE;
}
#endif // fcWindows
