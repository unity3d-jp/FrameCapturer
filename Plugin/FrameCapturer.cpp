#include "pch.h"
#include "FrameCapturer.h"
#include "fcGraphicsDevice.h"



#ifdef fcSupportEXR
#include "fcExrFile.h"

#ifdef fcDebug
#define fcTypeCheck(v) if(v==nullptr || *(fcEMagic*)v!=fcE_ExrContext) { fcBreak(); }
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
#include "fcGifFile.h"

#ifdef fcDebug
#define fcTypeCheck(v) if(v==nullptr || *(fcEMagic*)v!=fcE_GifContext) { fcBreak(); }
#else  // fcDebug
#define fcTypeCheck(v) 
#endif // fcDebug

//fcCreateGifContextT fcCreateGifContext;
fcIGifContext* fcCreateGifContext(fcGifConfig &conf, fcIGraphicsDevice *dev);

fcCLinkage fcExport fcIGifContext* fcGifCreateContext(fcGifConfig *conf)
{
    if (fcCreateGifContext == nullptr) {
        // todo
    }
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
