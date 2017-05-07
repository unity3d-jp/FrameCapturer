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
#include "Encoder/fcPngContext.h"

fcIPngContext* fcPngCreateContextImpl(const fcPngConfig *conf, fcIGraphicsDevice *dev);

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
#endif // fcSupportPNG


// -------------------------------------------------------------
// EXR Exporter
// -------------------------------------------------------------

#ifdef fcSupportEXR
#include "Encoder/fcExrContext.h"

fcAPI fcIExrContext* fcExrCreateContextImpl(const fcExrConfig *conf, fcIGraphicsDevice *dev);

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
#endif // fcSupportEXR


// -------------------------------------------------------------
// GIF Exporter
// -------------------------------------------------------------

#ifdef fcSupportGIF
#include "Encoder/fcGifContext.h"

fcIGifContext* fcGifCreateContextImpl(const fcGifConfig &conf, fcIGraphicsDevice *dev);


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

fcAPI bool fcGifAddFramePixels(fcIGifContext *ctx, const void *pixels, fcPixelFormat fmt, bool keyframe, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addFramePixels(pixels, fmt, keyframe, timestamp);
}
fcAPI bool fcGifAddFrameTexture(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, bool keyframe, fcTime timestamp)
{
    fcTraceFunc();
    if (!ctx) { return false; }
    return ctx->addFrameTexture(tex, fmt, keyframe, timestamp);
}
fcAPI int fcGifAddFrameTextureDeferred(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, bool keyframe, fcTime timestamp, int id)
{
    fcTraceFunc();
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->addFrameTexture(tex, fmt, keyframe, timestamp);
    }, id);
}
#endif // fcSupportGIF



// -------------------------------------------------------------
// MP4 Exporter
// -------------------------------------------------------------

#ifdef fcSupportMP4
#include "Encoder/fcMP4Context.h"

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
#endif // fcSupportMP4



// -------------------------------------------------------------
// WebM Exporter
// -------------------------------------------------------------

#ifdef fcSupportWebM
#include "Encoder/fcWebMContext.h"

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

#endif // fcSupportWebM



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
