#include "pch.h"
#include "fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"


// -------------------------------------------------------------
// Foundation
// -------------------------------------------------------------

namespace {
    std::string g_fcModulePath;
}

fcAPI void fcSetModulePath(const char *path)
{
    g_fcModulePath = path;
    DLLAddSearchPath(path);
}

fcAPI const char* fcGetModulePath()
{
    return !g_fcModulePath.empty() ? g_fcModulePath.c_str() : DLLGetDirectoryOfCurrentModule();
}

fcAPI fcTime fcGetTime()
{
    return GetCurrentTimeInSeconds();
}

fcAPI fcStream* fcCreateFileStream(const char *path)
{
    return new StdIOStream(new std::fstream(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc), true);
}
fcAPI fcStream* fcCreateMemoryStream()
{
    return new BufferStream(new Buffer(), true);
}
fcAPI fcStream* fcCreateCustomStream(void *obj, fcTellp_t tellp, fcSeekp_t seekp, fcWrite_t write)
{
    CustomStreamData csd;
    csd.obj = obj;
    csd.tellp = tellp;
    csd.seekp = seekp;
    csd.write = write;
    return new CustomStream(csd);
}

fcAPI void fcDestroyStream(fcStream *s)
{
    delete s;
}

fcAPI fcBufferData fcStreamGetBufferData(fcStream *s)
{
    fcBufferData ret;
    if (BufferStream *bs = dynamic_cast<BufferStream*>(s)) {
        ret.data = bs->get().data();
        ret.size = bs->get().size();
    }
    return ret;
}

fcAPI uint64_t fcStreamGetWrittenSize(fcStream *s)
{
    return s->tellp();
}


#ifndef fcStaticLink

typedef std::function<void()> fcDeferredCall;
namespace {
    std::mutex g_deferred_calls_mutex;
    std::vector<fcDeferredCall> g_deferred_calls;
}

fcAPI void fcGuardBegin()
{
    g_deferred_calls_mutex.lock();
}

fcAPI void fcGuardEnd()
{
    g_deferred_calls_mutex.unlock();
}

fcAPI int fcAddDeferredCall(const fcDeferredCall& dc, int id)
{
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

fcAPI void fcEraseDeferredCall(int id)
{
    if (id <= 0 || id >= (int)g_deferred_calls.size()) { return; }

    g_deferred_calls[id] = fcDeferredCall();
}

// **called from rendering thread**
fcAPI void fcCallDeferredCall(int id)
{
    std::unique_lock<std::mutex> l(g_deferred_calls_mutex);
    if (id <= 0 || id >= (int)g_deferred_calls.size()) { return; }

    auto& dc = g_deferred_calls[id];
    if (dc) { dc(); }
}
#endif // fcStaticLink



// -------------------------------------------------------------
// PNG Exporter
// -------------------------------------------------------------

#ifdef fcSupportPNG
#include "Encoder/fcPngContext.h"

#ifdef fcPNGSplitModule
    #define fcPNGModuleName  "FrameCapturer_PNG" fcDLLExt
    static module_t fcPngModule;
    fcPngCreateContextImplT fcPngCreateContextImpl;
#else
    fcAPI fcIPngContext* fcPngCreateContextImpl(const fcPngConfig *conf, fcIGraphicsDevice *dev);
#endif

fcAPI fcIPngContext* fcPngCreateContext(const fcPngConfig *conf)
{
#ifdef fcPNGSplitModule
    if (!fcPngModule) {
        fcPngModule = DLLLoad(fcPNGModuleName);
        if (fcPngModule) {
            (void*&)fcPngCreateContextImpl = DLLGetSymbol(fcPngModule, "fcPngCreateContextImpl");
        }
    }
    return fcPngCreateContextImpl ? fcPngCreateContextImpl(conf, fcGetGraphicsDevice()) : nullptr;
#else
    return fcPngCreateContextImpl(conf, fcGetGraphicsDevice());
#endif
}

fcAPI void fcPngDestroyContext(fcIPngContext *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcAPI bool fcPngExportPixels(fcIPngContext *ctx, const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY)
{
    if (!ctx) { return false; }
    return ctx->exportPixels(path, pixels, width, height, fmt, flipY);
}

fcAPI bool fcPngExportTexture(fcIPngContext *ctx, const char *path, void *tex, int width, int height, fcPixelFormat fmt, bool flipY)
{
    if (!ctx) { return false; }
    return ctx->exportTexture(path, tex, width, height, fmt, flipY);
}

#ifndef fcStaticLink
fcAPI int fcPngExportTextureDeferred(fcIPngContext *ctx, const char *path_, void *tex, int width, int height, fcPixelFormat fmt, bool flipY, int id)
{
    if (!ctx) { return 0; }

    std::string path = path_;
    return fcAddDeferredCall([=]() {
        ctx->exportTexture(path.c_str(), tex, width, height, fmt, flipY);
    }, id);
}
#endif // fcStaticLink

#endif // fcSupportPNG


// -------------------------------------------------------------
// EXR Exporter
// -------------------------------------------------------------

#ifdef fcSupportEXR
#include "Encoder/fcExrContext.h"

#ifdef fcEXRSplitModule
    #define fcEXRModuleName  "FrameCapturer_EXR" fcDLLExt
    static module_t fcExrModule;
    fcExrCreateContextImplT fcExrCreateContextImpl;
#else
    fcAPI fcIExrContext* fcExrCreateContextImpl(const fcExrConfig *conf, fcIGraphicsDevice *dev);
#endif


fcAPI fcIExrContext* fcExrCreateContext(const fcExrConfig *conf)
{
#ifdef fcEXRSplitModule
    if (!fcExrModule) {
        fcExrModule = DLLLoad(fcEXRModuleName);
        if (fcExrModule) {
            (void*&)fcExrCreateContextImpl = DLLGetSymbol(fcExrModule, "fcExrCreateContextImpl");
        }
    }
    return fcExrCreateContextImpl ? fcExrCreateContextImpl(conf, fcGetGraphicsDevice()) : nullptr;
#else
    return fcExrCreateContextImpl(conf, fcGetGraphicsDevice());
#endif
}

fcAPI void fcExrDestroyContext(fcIExrContext *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcAPI bool fcExrBeginFrame(fcIExrContext *ctx, const char *path, int width, int height)
{
    if (!ctx) { return false; }
    return ctx->beginFrame(path, width, height);
}

fcAPI bool fcExrAddLayerPixels(fcIExrContext *ctx, const void *pixels, fcPixelFormat fmt, int ch, const char *name, bool flipY)
{
    if (!ctx) { return false; }
    return ctx->addLayerPixels(pixels, fmt, ch, name, flipY);
}

fcAPI bool fcExrAddLayerTexture(fcIExrContext *ctx, void *tex, fcPixelFormat fmt, int ch, const char *name, bool flipY)
{
    if (!ctx) { return false; }
    return ctx->addLayerTexture(tex, fmt, ch, name, flipY);
}

fcAPI bool fcExrEndFrame(fcIExrContext *ctx)
{
    if (!ctx) { return false; }
    return ctx->endFrame();
}

#ifndef fcStaticLink
fcAPI int fcExrBeginFrameDeferred(fcIExrContext *ctx, const char *path_, int width, int height, int id)
{
    if (!ctx) { return 0; }
    std::string path = path_;
    return fcAddDeferredCall([=]() {
        return ctx->beginFrame(path.c_str(), width, height);
    }, id);
}

fcAPI int fcExrAddLayerTextureDeferred(fcIExrContext *ctx, void *tex, fcPixelFormat fmt, int ch, const char *name_, bool flipY, int id)
{
    if (!ctx) { return 0; }
    std::string name = name_;
    return fcAddDeferredCall([=]() {
        return ctx->addLayerTexture(tex, fmt, ch, name.c_str(), flipY);
    }, id);
}

fcAPI int fcExrEndFrameDeferred(fcIExrContext *ctx, int id)
{
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->endFrame();
    }, id);
}
#endif // fcStaticLink
#endif // fcSupportEXR


// -------------------------------------------------------------
// GIF Exporter
// -------------------------------------------------------------

#ifdef fcSupportGIF
#include "Encoder/fcGifContext.h"

#ifdef fcGIFSplitModule
    #define fcGIFModuleName  "FrameCapturer_GIF" fcDLLExt
    static module_t fcGifModule;
    fcGifCreateContextImplT fcGifCreateContextImpl;
#else
    fcAPI fcIGifContext* fcGifCreateContextImpl(const fcGifConfig &conf, fcIGraphicsDevice *dev);
#endif


fcAPI fcIGifContext* fcGifCreateContext(const fcGifConfig *conf)
{
#ifdef fcGIFSplitModule
    if (!fcGifModule) {
        fcGifModule = DLLLoad(fcGIFModuleName);
        if (fcGifModule) {
            (void*&)fcExrCreateContextImpl = DLLGetSymbol(fcGifModule, "fcGifCreateContextImpl");
        }
    }
    return fcExrCreateContextImpl ? fcExrCreateContextImpl(*conf, fcGetGraphicsDevice()) : nullptr;
#else
    return fcGifCreateContextImpl(*conf, fcGetGraphicsDevice());
#endif
}

fcAPI void fcGifDestroyContext(fcIGifContext *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcAPI bool fcGifAddFramePixels(fcIGifContext *ctx, const void *pixels, fcPixelFormat fmt, bool keyframe, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addFramePixels(pixels, fmt, keyframe, timestamp);
}
fcAPI bool fcGifAddFrameTexture(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, bool keyframe, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addFrameTexture(tex, fmt, keyframe, timestamp);
}
#ifndef fcStaticLink
fcAPI int fcGifAddFrameTextureDeferred(fcIGifContext *ctx, void *tex, fcPixelFormat fmt, bool keyframe, fcTime timestamp, int id)
{
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->addFrameTexture(tex, fmt, keyframe, timestamp);
    }, id);
}
#endif // fcStaticLink

fcAPI bool fcGifWrite(fcIGifContext *ctx, fcStream *stream, int begin_frame, int end_frame)
{
    if (!ctx || !stream) { return false; }
    return ctx->write(*stream, begin_frame, end_frame);
}

fcAPI void fcGifClearFrame(fcIGifContext *ctx)
{
    if (!ctx) { return; }
    ctx->clearFrame();
}

fcAPI int fcGifGetFrameCount(fcIGifContext *ctx)
{
    if (!ctx) { return 0; }
    return ctx->getFrameCount();
}

fcAPI void fcGifGetFrameData(fcIGifContext *ctx, void *tex, int frame)
{
    if (!ctx) { return; }
    return ctx->getFrameData(tex, frame);
}

fcAPI int fcGifGetExpectedDataSize(fcIGifContext *ctx, int begin_frame, int end_frame)
{
    if (!ctx) { return 0; }
    return ctx->getExpectedDataSize(begin_frame, end_frame);
}

fcAPI void fcGifEraseFrame(fcIGifContext *ctx, int begin_frame, int end_frame)
{
    if (!ctx) { return; }
    ctx->eraseFrame(begin_frame, end_frame);
}
#endif // fcSupportGIF



// -------------------------------------------------------------
// MP4 Exporter
// -------------------------------------------------------------

#ifdef fcSupportMP4
#include "Encoder/fcMP4Context.h"

#ifdef fcMP4SplitModule
    #define fcMP4ModuleName  "FrameCapturer_MP4" fcDLLExt
    namespace {
    #define decl(Name) Name##_t Name;
        fcMP4EachFunctions(decl)
    #undef decl

        module_t fcMP4Module;
        void fcMP4InitializeModule()
        {
            if (!fcMP4Module) {
                fcMP4Module = DLLLoad(fcMP4ModuleName);
                if (fcMP4Module) {
    #define imp(Name) (void*&)Name = DLLGetSymbol(fcMP4Module, #Name);
                    fcMP4EachFunctions(imp)
    #undef imp
                }
            }
        }
    }
#endif // fcMP4SplitModule

fcAPI void fcMP4SetFAACPackagePath(const char *path)
{
#ifdef fcMP4SplitModule
    fcMP4InitializeModule();
    if (fcMP4SetFAACPackagePathImpl) {
        fcMP4SetFAACPackagePathImpl(path);
    }
#else
    return fcMP4SetFAACPackagePathImpl(path);
#endif
}

fcAPI bool fcMP4DownloadCodecBegin()
{
#ifdef fcMP4SplitModule
    fcMP4InitializeModule();
    if (fcMP4SetModulePathImpl && fcMP4DownloadCodecBeginImpl) {
        fcMP4SetModulePathImpl(fcGetModulePath());
        return fcMP4DownloadCodecBeginImpl();
    }
    return false;
#else
    return fcMP4DownloadCodecBeginImpl();
#endif
}

fcAPI fcDownloadState fcMP4DownloadCodecGetState()
{
#ifdef fcMP4SplitModule
    fcMP4InitializeModule();
    if (fcMP4DownloadCodecGetStateImpl) {
        return fcMP4DownloadCodecGetStateImpl();
    }
    return fcDownloadState_Error;
#else
    return fcMP4DownloadCodecGetStateImpl();
#endif
}


fcAPI fcIMP4Context* fcMP4CreateContext(fcMP4Config *conf)
{
#ifdef fcMP4SplitModule
    fcMP4InitializeModule();
    if (fcMP4CreateContextImpl) {
        return fcMP4CreateContextImpl(*conf, fcGetGraphicsDevice());
    }
    return nullptr;
#else
    return fcMP4CreateContextImpl(*conf, fcGetGraphicsDevice());
#endif
}

fcAPI fcIMP4Context* fcMP4CreateOSEncoderContext(fcMP4Config *conf, const char *out_path)
{
#ifdef fcMP4SplitModule
    fcMP4InitializeModule();
    if (fcMP4CreateOSEncoderContextImpl) {
        return fcMP4CreateOSEncoderContextImpl(*conf, fcGetGraphicsDevice(), out_path);
    }
    return nullptr;
#else
    return fcMP4CreateOSEncoderContextImpl(*conf, fcGetGraphicsDevice(), out_path);
#endif
}

fcAPI void fcMP4DestroyContext(fcIMP4Context *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcAPI const char* fcMP4GetVideoEncoderInfo(fcIMP4Context *ctx)
{
    if (!ctx) { return ""; }
    return ctx->getVideoEncoderInfo();
}
fcAPI const char* fcMP4GetAudioEncoderInfo(fcIMP4Context *ctx)
{
    if (!ctx) { return ""; }
    return ctx->getAudioEncoderInfo();
}

fcAPI void fcMP4AddOutputStream(fcIMP4Context *ctx, fcStream *stream)
{
    if (!ctx) { return; }
    ctx->addOutputStream(stream);
}

fcAPI bool fcMP4AddVideoFramePixels(fcIMP4Context *ctx, const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addVideoFramePixels(pixels, fmt, timestamp);
}
fcAPI bool fcMP4AddVideoFrameTexture(fcIMP4Context *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addVideoFrameTexture(tex, fmt, timestamp);
}
#ifndef fcStaticLink
fcAPI int fcMP4AddVideoFrameTextureDeferred(fcIMP4Context *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp, int id)
{
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->addVideoFrameTexture(tex, fmt, timestamp);
    }, id);
}
#endif // fcStaticLink

fcAPI bool fcMP4AddAudioFrame(fcIMP4Context *ctx, const float *samples, int num_samples, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addAudioFrame(samples, num_samples, timestamp);
}
#endif // fcSupportMP4



// -------------------------------------------------------------
// WebM Exporter
// -------------------------------------------------------------

#ifdef fcSupportWebM
#include "Encoder/fcWebMContext.h"

#ifdef fcWebMSplitModule
    #define fcWebMModuleName  "FrameCapturer_WebM" fcDLLExt
    namespace {
        fcWebMCreateContextImpl_t fcWebMCreateContextImpl;

        module_t fcWebMModule;
        void fcWebMInitializeModule()
        {
            if (!fcWebMModule) {
                fcWebMModule = DLLLoad(fcWebMModuleName);
                if (fcWebMModule) {
                    (void*&)fcWebMCreateContextImpl = DLLGetSymbol(fcWebMModule, "fcWebMCreateContextImpl");
                }
            }
        }
    }
#endif // fcSupportWebM

fcAPI fcIWebMContext* fcWebMCreateContext(fcWebMConfig *conf)
{
#ifdef fcWebMSplitModule
    fcWebMInitializeModule();
    if (fcWebMCreateContextImpl) {
        return fcWebMCreateContextImpl(*conf, fcGetGraphicsDevice());
    }
    return nullptr;
#else
    return fcWebMCreateContextImpl(*conf, fcGetGraphicsDevice());
#endif
}

fcAPI void fcWebMDestroyContext(fcIWebMContext *ctx)
{
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
    if (!ctx) { return false; }
    return ctx->addVideoFramePixels(pixels, fmt, timestamp);
}

fcAPI bool fcWebMAddVideoFrameTexture(fcIWebMContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addVideoFrameTexture(tex, fmt, timestamp);
}

#ifndef fcStaticLink
fcAPI int fcWebMAddVideoFrameTextureDeferred(fcIWebMContext *ctx, void *tex, fcPixelFormat fmt, fcTime timestamp, int id)
{
    if (!ctx) { return 0; }
    return fcAddDeferredCall([=]() {
        return ctx->addVideoFrameTexture(tex, fmt, timestamp);
    }, id);
}
#endif // fcStaticLink

fcAPI bool fcWebMAddAudioFrame(fcIWebMContext *ctx, const float *samples, int num_samples, fcTime timestamp)
{
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
        // add dll search path to load additional modules (FrameCapturer_MP4.dll etc).
        DLLAddSearchPath(DLLGetDirectoryOfCurrentModule());

        // initialize graphics device
        fcGfxForceInitialize();
    }
    return TRUE;
}
#endif // fcWindows
