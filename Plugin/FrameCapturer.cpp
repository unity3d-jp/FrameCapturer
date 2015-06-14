#include "pch.h"
#include "FrameCapturer.h"
#include "fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"



#ifdef fcSupportEXR
#include "Encoder/fcExrFile.h"

#ifdef fcEXRSplitModule
    #define fcEXRModuleName  "FrameCapturer_EXR" fcModuleExt
    static module_t fcExrModule;
    fcExrCreateContextImplT fcExrCreateContextImpl;
#else
    fcCLinkage fcExport fcIExrContext* fcExrCreateContextImpl(fcExrConfig &conf, fcIGraphicsDevice *dev);
#endif


fcCLinkage fcExport fcIExrContext* fcExrCreateContext(fcExrConfig *conf)
{
#ifdef fcEXRSplitModule
    if (!fcExrModule) {
        fcExrModule = module_load(fcEXRModuleName);
        if (fcExrModule) {
            (void*&)fcExrCreateContextImpl = module_getsymbol(fcExrModule, "fcExrCreateContextImpl");
        }
    }
    return fcExrCreateContextImpl ? fcExrCreateContextImpl(*conf, fcGetGraphicsDevice()) : nullptr;
#else
    return fcExrCreateContextImpl(*conf, fcGetGraphicsDevice());
#endif
}

fcCLinkage fcExport void fcExrDestroyContext(fcIExrContext *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcCLinkage fcExport bool fcExrBeginFrame(fcIExrContext *ctx, const char *path, int width, int height)
{
    if (!ctx) { return false; }
    return ctx->beginFrame(path, width, height);
}

fcCLinkage fcExport bool fcExrAddLayer(fcIExrContext *ctx, void *tex, fcETextureFormat fmt, int ch, const char *name)
{
    if (!ctx) { return false; }
    return ctx->addLayer(tex, fmt, ch, name);
}

fcCLinkage fcExport bool fcExrEndFrame(fcIExrContext *ctx)
{
    if (!ctx) { return false; }
    return ctx->endFrame();
}
#endif // fcSupportEXR



#ifdef fcSupportGIF
#include "Encoder/fcGifFile.h"

#ifdef fcGIFSplitModule
    #define fcGIFModuleName  "FrameCapturer_GIF" fcModuleExt
    static module_t fcGifModule;
    fcGifCreateContextImplT fcGifCreateContextImpl;
#else
    fcCLinkage fcExport fcIGifContext* fcGifCreateContextImpl(fcGifConfig &conf, fcIGraphicsDevice *dev);
#endif


fcCLinkage fcExport fcIGifContext* fcGifCreateContext(fcGifConfig *conf)
{
#ifdef fcGIFSplitModule
    if (!fcGifModule) {
        fcGifModule = module_load(fcGIFModuleName);
        if (fcGifModule) {
            (void*&)fcExrCreateContextImpl = module_getsymbol(fcGifModule, "fcGifCreateContextImpl");
        }
    }
    return fcExrCreateContextImpl ? fcExrCreateContextImpl(*conf, fcGetGraphicsDevice()) : nullptr;
#else
    return fcGifCreateContextImpl(*conf, fcGetGraphicsDevice());
#endif
}

fcCLinkage fcExport void fcGifDestroyContext(fcIGifContext *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcCLinkage fcExport bool fcGifAddFrame(fcIGifContext *ctx, void *tex)
{
    if (!ctx) { return false; }
    return ctx->addFrame(tex);
}

fcCLinkage fcExport void fcGifClearFrame(fcIGifContext *ctx)
{
    if (!ctx) { return; }
    ctx->clearFrame();
}

fcCLinkage fcExport bool fcGifWriteFile(fcIGifContext *ctx, const char *path, int begin_frame, int end_frame)
{
    if (!ctx) { return false; }
    return ctx->writeFile(path, begin_frame, end_frame);
}

fcCLinkage fcExport int fcGifWriteMemory(fcIGifContext *ctx, void *buf, int begin_frame, int end_frame)
{
    if (!ctx) { return 0; }
    return ctx->writeMemory(buf, begin_frame, end_frame);
}

fcCLinkage fcExport int fcGifGetFrameCount(fcIGifContext *ctx)
{
    if (!ctx) { return 0; }
    return ctx->getFrameCount();
}

fcCLinkage fcExport void fcGifGetFrameData(fcIGifContext *ctx, void *tex, int frame)
{
    if (!ctx) { return; }
    return ctx->getFrameData(tex, frame);
}

fcCLinkage fcExport int fcGifGetExpectedDataSize(fcIGifContext *ctx, int begin_frame, int end_frame)
{
    if (!ctx) { return 0; }
    return ctx->getExpectedDataSize(begin_frame, end_frame);
}

fcCLinkage fcExport void fcGifEraseFrame(fcIGifContext *ctx, int begin_frame, int end_frame)
{
    if (!ctx) { return; }
    ctx->eraseFrame(begin_frame, end_frame);
}
#endif // fcSupportGIF




#ifdef fcSupportMP4
#include "Encoder/fcMP4File.h"

#ifdef fcMP4SplitModule
    #define fcMP4ModuleName  "FrameCapturer_MP4" fcModuleExt
    static module_t fcMP4Module;
    static fcMP4CreateContextImplT fcMP4CreateContextImpl;
#else
    fcCLinkage fcExport fcIMP4Context* fcMP4CreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice *dev);
#endif


fcCLinkage fcExport fcIMP4Context* fcMP4CreateContext(fcMP4Config *conf)
{
#ifdef fcMP4SplitModule
    if (!fcMP4Module) {
        fcMP4Module = module_load(fcMP4ModuleName);
        if (fcMP4Module) {
            (void*&)fcMP4CreateContextImpl = module_getsymbol(fcMP4Module, "fcMP4CreateContextImpl");
        }
    }
    return fcMP4CreateContextImpl ? fcMP4CreateContextImpl(*conf, fcGetGraphicsDevice()) : nullptr;
#else
    fcMP4CreateContextImpl(*conf, fcGetGraphicsDevice());
#endif
}

fcCLinkage fcExport void fcMP4DestroyContext(fcIMP4Context *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcCLinkage fcExport bool fcMP4AddFrameTexture(fcIMP4Context *ctx, void *tex)
{
    if (!ctx) { return false; }
    return ctx->addFrameTexture(tex);
}

fcCLinkage fcExport bool fcMP4AddFramePixels(fcIMP4Context *ctx, void *pixels, fcEColorSpace cs)
{
    if (!ctx) { return false; }
    return ctx->addFramePixels(pixels, cs);
}

fcCLinkage fcExport void fcMP4ClearFrame(fcIMP4Context *ctx)
{
    if (!ctx) { return; }
    ctx->clearFrame();
}

fcCLinkage fcExport bool fcMP4WriteFile(fcIMP4Context *ctx, const char *path, int begin_frame, int end_frame)
{
    if (!ctx) { return false; }
    return ctx->writeFile(path, begin_frame, end_frame);
}

fcCLinkage fcExport int fcMP4WriteMemory(fcIMP4Context *ctx, void *buf, int begin_frame, int end_frame)
{
    if (!ctx) { return 0; }
    return ctx->writeMemory(buf, begin_frame, end_frame);
}

fcCLinkage fcExport int fcMP4GetFrameCount(fcIMP4Context *ctx)
{
    if (!ctx) { return 0; }
    return ctx->getFrameCount();
}

fcCLinkage fcExport void fcMP4GetFrameData(fcIMP4Context *ctx, void *tex, int frame)
{
    if (!ctx) { return; }
    return ctx->getFrameData(tex, frame);
}

fcCLinkage fcExport int fcMP4GetExpectedDataSize(fcIMP4Context *ctx, int begin_frame, int end_frame)
{
    if (!ctx) { return 0; }
    return ctx->getExpectedDataSize(begin_frame, end_frame);
}

fcCLinkage fcExport void fcMP4EraseFrame(fcIMP4Context *ctx, int begin_frame, int end_frame)
{
    if (!ctx) { return; }
    ctx->eraseFrame(begin_frame, end_frame);
}

#endif // fcSupportMP4
