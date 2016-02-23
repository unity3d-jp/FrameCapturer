#include "pch.h"
#include "fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"


// -------------------------------------------------------------
// Foundation
// -------------------------------------------------------------

static std::string g_module_path;

fcCLinkage fcExport void fcSetModulePath(const char *path)
{
    g_module_path = path;
    DLLAddSearchPath(path);
}

fcCLinkage fcExport const char* fcGetModulePath()
{
    return !g_module_path.empty() ? g_module_path.c_str() : DLLGetDirectoryOfCurrentModule();
}


fcCLinkage fcExport uint64_t fcGetTime()
{
    return GetCurrentTimeNanosec();
}

fcCLinkage fcExport uint64_t fcSecondsToTimestamp(double sec)
{
    // timestamp is nanoseconds
    return uint64_t(sec * 1e+9);
}


// -------------------------------------------------------------
// PNG Exporter
// -------------------------------------------------------------

#ifdef fcSupportPNG
#include "Encoder/fcPngFile.h"

#ifdef fcPNGSplitModule
    #define fcPNGModuleName  "FrameCapturer_PNG" fcDLLExt
    static module_t fcPngModule;
    fcPngCreateContextImplT fcPngCreateContextImpl;
#else
    fcCLinkage fcExport fcIPngContext* fcPngCreateContextImpl(const fcPngConfig *conf, fcIGraphicsDevice *dev);
#endif

fcCLinkage fcExport fcIPngContext* fcPngCreateContext(const fcPngConfig *conf)
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

fcCLinkage fcExport void fcPngDestroyContext(fcIPngContext *ctx)
{
    ctx->release();
}

fcCLinkage fcExport bool fcPngExportTexture(fcIPngContext *ctx, const char *path, void *tex, int width, int height, fcTextureFormat fmt, bool flipY)
{
    return ctx->exportTexture(path, tex, width, height, fmt, flipY);
}

fcCLinkage fcExport bool fcPngExportPixels(fcIPngContext *ctx, const char *path, const void *pixels, int width, int height, fcPixelFormat fmt, bool flipY)
{
    return ctx->exportPixels(path, pixels, width, height, fmt, flipY);
}

#endif // #fcSupportPNG


// -------------------------------------------------------------
// EXR Exporter
// -------------------------------------------------------------

#ifdef fcSupportEXR
#include "Encoder/fcExrFile.h"

#ifdef fcEXRSplitModule
    #define fcEXRModuleName  "FrameCapturer_EXR" fcDLLExt
    static module_t fcExrModule;
    fcExrCreateContextImplT fcExrCreateContextImpl;
#else
    fcCLinkage fcExport fcIExrContext* fcExrCreateContextImpl(const fcExrConfig *conf, fcIGraphicsDevice *dev);
#endif


fcCLinkage fcExport fcIExrContext* fcExrCreateContext(const fcExrConfig *conf)
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

fcCLinkage fcExport bool fcExrAddLayerTexture(fcIExrContext *ctx, void *tex, fcTextureFormat fmt, int ch, const char *name, bool flipY)
{
    if (!ctx) { return false; }
    return ctx->addLayerTexture(tex, fmt, ch, name, flipY);
}

fcCLinkage fcExport bool fcExrAddLayerPixels(fcIExrContext *ctx, const void *pixels, fcPixelFormat fmt, int ch, const char *name, bool flipY)
{
    if (!ctx) { return false; }
    return ctx->addLayerPixels(pixels, fmt, ch, name, flipY);
}

fcCLinkage fcExport bool fcExrEndFrame(fcIExrContext *ctx)
{
    if (!ctx) { return false; }
    return ctx->endFrame();
}
#endif // fcSupportEXR


// -------------------------------------------------------------
// GIF Exporter
// -------------------------------------------------------------

#ifdef fcSupportGIF
#include "Encoder/fcGifFile.h"

#ifdef fcGIFSplitModule
    #define fcGIFModuleName  "FrameCapturer_GIF" fcDLLExt
    static module_t fcGifModule;
    fcGifCreateContextImplT fcGifCreateContextImpl;
#else
    fcCLinkage fcExport fcIGifContext* fcGifCreateContextImpl(fcGifConfig &conf, fcIGraphicsDevice *dev);
#endif


fcCLinkage fcExport fcIGifContext* fcGifCreateContext(fcGifConfig *conf)
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



// -------------------------------------------------------------
// MP4 Exporter
// -------------------------------------------------------------

fcCLinkage fcExport fcStream* fcCreateFileStream(const char *path)
{
    return new StdIOStream(new std::fstream(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc), true);
}
fcCLinkage fcExport fcStream* fcCreateMemoryStream()
{
    return new BufferStream(new Buffer(), true);
}
fcCLinkage fcExport fcStream* fcCreateCustomStream(void *obj, fcTellp_t tellp, fcSeekp_t seekp, fcWrite_t write)
{
    CustomStreamData csd;
    csd.obj = obj;
    csd.tellp = tellp;
    csd.seekp = seekp;
    csd.write = write;
    return new CustomStream(csd);
}

fcCLinkage fcExport void fcDestroyStream(fcStream *s)
{
    delete s;
}

fcCLinkage fcExport fcBufferData fcStreamGetBufferData(fcStream *s)
{
    fcBufferData ret;
    if (BufferStream *bs = dynamic_cast<BufferStream*>(s)) {
        ret.data = bs->get().ptr();
        ret.size = bs->get().size();
    }
    return ret;
}

fcCLinkage fcExport uint64_t fcStreamGetWrittenSize(fcStream *s)
{
    return s->tellp();
}



#ifdef fcSupportMP4
#include "Encoder/fcMP4File.h"


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
#endif

#ifdef fcMP4SplitModule
#endif // fcMP4SplitModule

fcCLinkage fcExport bool fcMP4DownloadCodecBegin()
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

fcCLinkage fcExport fcDownloadState fcMP4DownloadCodecGetState()
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


fcCLinkage fcExport fcIMP4Context* fcMP4CreateContext(fcMP4Config *conf)
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

fcCLinkage fcExport void fcMP4DestroyContext(fcIMP4Context *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcCLinkage fcExport void fcMP4AddOutputStream(fcIMP4Context *ctx, fcStream *stream)
{
    if (!ctx) { return; }
    ctx->addOutputStream(stream);
}

fcCLinkage fcExport bool fcMP4AddVideoFrameTexture(fcIMP4Context *ctx, void *tex, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addVideoFrameTexture(tex, timestamp);
}

fcCLinkage fcExport bool fcMP4AddVideoFramePixels(fcIMP4Context *ctx, const void *pixels, fcColorSpace cs, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addVideoFramePixels(pixels, cs, timestamp);
}

fcCLinkage fcExport bool fcMP4AddAudioFrame(fcIMP4Context *ctx, const float *samples, int num_samples, fcTime timestamp)
{
    if (!ctx) { return false; }
    return ctx->addAudioFrame(samples, num_samples, timestamp);
}
#endif // fcSupportMP4


#ifdef fcWindows

#include <windows.h>

fcCLinkage fcExport void UnitySetGraphicsDevice(void* device, int deviceType, int eventType);
typedef fcIGraphicsDevice* (*fcGetGraphicsDeviceT)();

BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call == DLL_PROCESS_ATTACH)
    {
        // add dll search path to load additional modules (FrameCapturer_MP4.dll etc).
        DLLAddSearchPath(DLLGetDirectoryOfCurrentModule());

#ifndef fcMaster
        // PatchLibrary で突っ込まれたモジュールは UnitySetGraphicsDevice() が呼ばれないので、
        // DLL_PROCESS_ATTACH のタイミングで先にロードされているモジュールからデバイスをもらって同等の処理を行う。
        HMODULE m = ::GetModuleHandleA("FrameCapturer.dll");
        if (m) {
            auto proc = (fcGetGraphicsDeviceT)::GetProcAddress(m, "fcGetGraphicsDevice");
            if (proc) {
                fcIGraphicsDevice *dev = proc();
                if (dev) {
                    UnitySetGraphicsDevice(dev->getDevicePtr(), dev->getDeviceType(), kGfxDeviceEventInitialize);
                }
            }
        }
#endif // fcMaster
    }
    return TRUE;
}

// prevent "DllMain already defined in MSVCRT.lib"
#ifdef _X86_
extern "C" { int _afxForceUSRDLL; }
#else
extern "C" { int __afxForceUSRDLL; }
#endif
#endif // fcWindows
