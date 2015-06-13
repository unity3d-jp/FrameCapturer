#include "pch.h"
#include "FrameCapturer.h"
#include "GraphicsDevice/fcGraphicsDevice.h"



#ifdef fcSupportEXR
#include "Encoder/fcExrFile.h"

#ifdef fcDebug
#define fcTypeCheck(v) if(v==nullptr || *(fcEMagic*)((void**)v+1)!=fcE_ExrContext) { fcBreak(); }
#else  // fcDebug
#define fcTypeCheck(v) 
#endif // fcDebug

// exr エクスポート部分を dll に分離することを見越して interface 化している。
// (exr 必要な人は少ない上、exr のライブラリをリンクすると 2MB 以上バイナリ増えるので…)

//fcCreateExrContextT fcCreateExrContext;
fcIExrContext* fcCreateExrContext(fcExrConfig &conf, fcIGraphicsDevice *dev);

fcCLinkage fcExport fcIExrContext* fcExrCreateContext(fcExrConfig *conf)
{
    return fcCreateExrContext(*conf, fcGetGraphicsDevice());
}

fcCLinkage fcExport void fcExrDestroyContext(fcIExrContext *ctx)
{
    if (ctx == nullptr) { return; }
    fcTypeCheck(ctx);
    ctx->release();
}

fcCLinkage fcExport bool fcExrBeginFrame(fcIExrContext *ctx, const char *path, int width, int height)
{
    fcTypeCheck(ctx);
    return ctx->beginFrame(path, width, height);
}

fcCLinkage fcExport bool fcExrAddLayer(fcIExrContext *ctx, void *tex, fcETextureFormat fmt, int ch, const char *name)
{
    fcTypeCheck(ctx);
    return ctx->addLayer(tex, fmt, ch, name);
}

fcCLinkage fcExport bool fcExrEndFrame(fcIExrContext *ctx)
{
    fcTypeCheck(ctx);
    return ctx->endFrame();
}
#undef fcTypeCheck

#endif // fcSupportEXR



#ifdef fcSupportGIF
#include "Encoder/fcGifFile.h"

#ifdef fcDebug
#define fcTypeCheck(v) if(v==nullptr || *(fcEMagic*)((void**)v+1)!=fcE_GifContext) { fcBreak(); }
#else  // fcDebug
#define fcTypeCheck(v) 
#endif // fcDebug

//fcCreateGifContextT fcCreateGifContext;
fcIGifContext* fcCreateGifContext(fcGifConfig &conf, fcIGraphicsDevice *dev);

fcCLinkage fcExport fcIGifContext* fcGifCreateContext(fcGifConfig *conf)
{
    return fcCreateGifContext(*conf, fcGetGraphicsDevice());
}

fcCLinkage fcExport void fcGifDestroyContext(fcIGifContext *ctx)
{
    if (ctx == nullptr) { return; }
    fcTypeCheck(ctx);
    ctx->release();
}

fcCLinkage fcExport bool fcGifAddFrame(fcIGifContext *ctx, void *tex)
{
    fcTypeCheck(ctx);
    return ctx->addFrame(tex);
}

fcCLinkage fcExport void fcGifClearFrame(fcIGifContext *ctx)
{
    fcTypeCheck(ctx);
    ctx->clearFrame();
}

fcCLinkage fcExport bool fcGifWriteFile(fcIGifContext *ctx, const char *path, int begin_frame, int end_frame)
{
    fcTypeCheck(ctx);
    return ctx->writeFile(path, begin_frame, end_frame);
}

fcCLinkage fcExport int fcGifWriteMemory(fcIGifContext *ctx, void *buf, int begin_frame, int end_frame)
{
    fcTypeCheck(ctx);
    return ctx->writeMemory(buf, begin_frame, end_frame);
}

fcCLinkage fcExport int fcGifGetFrameCount(fcIGifContext *ctx)
{
    fcTypeCheck(ctx);
    return ctx->getFrameCount();
}

fcCLinkage fcExport void fcGifGetFrameData(fcIGifContext *ctx, void *tex, int frame)
{
    fcTypeCheck(ctx);
    return ctx->getFrameData(tex, frame);
}

fcCLinkage fcExport int fcGifGetExpectedDataSize(fcIGifContext *ctx, int begin_frame, int end_frame)
{
    fcTypeCheck(ctx);
    return ctx->getExpectedDataSize(begin_frame, end_frame);
}

fcCLinkage fcExport void fcGifEraseFrame(fcIGifContext *ctx, int begin_frame, int end_frame)
{
    fcTypeCheck(ctx);
    ctx->eraseFrame(begin_frame, end_frame);
}
#undef fcTypeCheck

#endif // fcSupportGIF




#ifdef fcSupportMP4
#include "Encoder/fcMP4File.h"

#ifdef fcDebug
#define fcTypeCheck(v) if(v==nullptr || *(fcEMagic*)((void**)v+1)!=fcE_MP4Context) { fcBreak(); }
#else  // fcDebug
#define fcTypeCheck(v) 
#endif // fcDebug

fcIMP4Context* fcCreateMP4Context(fcMP4Config &conf, fcIGraphicsDevice *dev);

fcCLinkage fcExport fcIMP4Context* fcMP4CreateContext(fcMP4Config *conf)
{
    return fcCreateMP4Context(*conf, fcGetGraphicsDevice());
}

fcCLinkage fcExport void fcMP4DestroyContext(fcIMP4Context *ctx)
{
    if (ctx == nullptr) { return; }
    fcTypeCheck(ctx);
    ctx->release();
}

fcCLinkage fcExport bool fcMP4AddFrameTexture(fcIMP4Context *ctx, void *tex)
{
    fcTypeCheck(ctx);
    return ctx->addFrameTexture(tex);
}

fcCLinkage fcExport bool fcMP4AddFramePixels(fcIMP4Context *ctx, void *pixels, fcEColorSpace cs)
{
    fcTypeCheck(ctx);
    return ctx->addFramePixels(pixels, cs);
}

fcCLinkage fcExport void fcMP4ClearFrame(fcIMP4Context *ctx)
{
    fcTypeCheck(ctx);
    ctx->clearFrame();
}

fcCLinkage fcExport bool fcMP4WriteFile(fcIMP4Context *ctx, const char *path, int begin_frame, int end_frame)
{
    fcTypeCheck(ctx);
    return ctx->writeFile(path, begin_frame, end_frame);
}

fcCLinkage fcExport int fcMP4WriteMemory(fcIMP4Context *ctx, void *buf, int begin_frame, int end_frame)
{
    fcTypeCheck(ctx);
    return ctx->writeMemory(buf, begin_frame, end_frame);
}

fcCLinkage fcExport int fcMP4GetFrameCount(fcIMP4Context *ctx)
{
    fcTypeCheck(ctx);
    return ctx->getFrameCount();
}

fcCLinkage fcExport void fcMP4GetFrameData(fcIMP4Context *ctx, void *tex, int frame)
{
    fcTypeCheck(ctx);
    return ctx->getFrameData(tex, frame);
}

fcCLinkage fcExport int fcMP4GetExpectedDataSize(fcIMP4Context *ctx, int begin_frame, int end_frame)
{
    fcTypeCheck(ctx);
    return ctx->getExpectedDataSize(begin_frame, end_frame);
}

fcCLinkage fcExport void fcMP4EraseFrame(fcIMP4Context *ctx, int begin_frame, int end_frame)
{
    fcTypeCheck(ctx);
    ctx->eraseFrame(begin_frame, end_frame);
}
#undef fcTypeCheck

#endif // fcSupportMP4
