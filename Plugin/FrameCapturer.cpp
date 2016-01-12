#include "pch.h"
#include "FrameCapturer.h"
#include "fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"


// -------------------------------------------------------------
// Foundation
// -------------------------------------------------------------

static std::vector<std::string> g_dll_search_paths;
static std::vector<const char*> g_dll_search_paths_ptr;

fcCLinkage fcExport void fcAddDLLSearchPath(const char *path)
{
    g_dll_search_paths.emplace_back(std::string(path));

    g_dll_search_paths_ptr.clear();
    for (auto &v : g_dll_search_paths) {
        g_dll_search_paths_ptr.push_back(v.c_str());
    }
    g_dll_search_paths_ptr.push_back(nullptr);
}

fcCLinkage fcExport const char** fcGetDLLSearchPaths()
{
    if (g_dll_search_paths_ptr.empty()) { g_dll_search_paths_ptr.push_back(nullptr); }
    return &g_dll_search_paths_ptr[0];
}


fcCLinkage fcExport uint64_t fcMakeTimestamp()
{
    return GetCurrentTimeNanosec();
}

fcCLinkage fcExport uint64_t fcSecondsToTimestamp(double sec)
{
    // timestamp is nanoseconds
    return uint64_t(sec * 1e+9);
}


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
    fcCLinkage fcExport fcIExrContext* fcExrCreateContextImpl(fcExrConfig &conf, fcIGraphicsDevice *dev);
#endif


fcCLinkage fcExport fcIExrContext* fcExrCreateContext(fcExrConfig *conf)
{
#ifdef fcEXRSplitModule
    if (!fcExrModule) {
        fcExrModule = DLLLoad(fcEXRModuleName);
        if (fcExrModule) {
            (void*&)fcExrCreateContextImpl = DLLGetSymbol(fcExrModule, "fcExrCreateContextImpl");
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

#ifdef fcSupportMP4
#include "Encoder/fcMP4File.h"

#ifdef fcMP4SplitModule
    #define fcMP4ModuleName  "FrameCapturer_MP4" fcDLLExt
    static module_t fcMP4Module;
    static fcMP4DownloadCodecImplT fcMP4DownloadCodecImpl;
    static fcMP4CreateContextImplT fcMP4CreateContextImpl;
#else
    fcCLinkage fcExport bool fcMP4DownloadCodecImpl(fcDownloadCallback cb)
    fcCLinkage fcExport fcIMP4Context* fcMP4CreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice *dev);
#endif

#ifdef fcMP4SplitModule
static void fcMP4InitializeModule()
{
    if (!fcMP4Module) {
        fcMP4Module = DLLLoad(fcMP4ModuleName);
        if (fcMP4Module) {
            (void*&)fcMP4DownloadCodecImpl = DLLGetSymbol(fcMP4Module, "fcMP4DownloadCodecImpl");
            (void*&)fcMP4CreateContextImpl = DLLGetSymbol(fcMP4Module, "fcMP4CreateContextImpl");
        }
    }
}
#endif // fcMP4SplitModule

fcCLinkage fcExport bool fcMP4DownloadCodec(fcDownloadCallback cb)
{
#ifdef fcMP4SplitModule
    fcMP4InitializeModule();
    return fcMP4DownloadCodecImpl ? fcMP4DownloadCodecImpl(cb) : false;
#else
    return fcMP4DownloadCodecImpl(cb);
#endif
}


fcCLinkage fcExport fcIMP4Context* fcMP4CreateContext(fcMP4Config *conf)
{
#ifdef fcMP4SplitModule
    fcMP4InitializeModule();
    return fcMP4CreateContextImpl ? fcMP4CreateContextImpl(*conf, fcGetGraphicsDevice()) : nullptr;
#else
    return fcMP4CreateContextImpl(*conf, fcGetGraphicsDevice());
#endif
}

fcCLinkage fcExport void fcMP4DestroyContext(fcIMP4Context *ctx)
{
    if (!ctx) { return; }
    ctx->release();
}

fcCLinkage fcExport bool fcMP4AddVideoFrameTexture(fcIMP4Context *ctx, void *tex)
{
    if (!ctx) { return false; }
    return ctx->addVideoFrameTexture(tex);
}

fcCLinkage fcExport bool fcMP4AddVideoFramePixels(fcIMP4Context *ctx, void *pixels, fcColorSpace cs)
{
    if (!ctx) { return false; }
    return ctx->addVideoFramePixels(pixels, cs);
}

fcCLinkage fcExport bool fcMP4AddAudioSamples(fcIMP4Context *ctx, const float *samples, int num_samples)
{
    if (!ctx) { return false; }
    return ctx->addAudioFrame(samples, num_samples);
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


#ifdef fcWindows

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call == DLL_PROCESS_ATTACH)
    {
        static bool s_is_first = true;
        if (s_is_first) {
            s_is_first = false;

            // add dll search path to load additional modules (FrameCapturer_MP4.dll etc).
            // get path of this module and add it to PATH environment variable
            std::string path;
            path.resize(1024 * 64);

            DWORD ret = ::GetEnvironmentVariableA("PATH", &path[0], path.size());
            path.resize(ret);
            {
                char path_to_this_module[MAX_PATH];
                HMODULE mod = 0;
                ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&DllMain, &mod);
                DWORD size = ::GetModuleFileNameA(mod, path_to_this_module, sizeof(path_to_this_module));
                for (int i = size - 1; i >= 0; --i) {
                    if (path_to_this_module[i] == '\\') {
                        path_to_this_module[i] = '\0';
                        break;
                    }
                }
                path += ";";
                path += path_to_this_module;
            }
            ::SetEnvironmentVariableA("PATH", path.c_str());
        }
    }
    else if (reason_for_call == DLL_PROCESS_DETACH)
    {
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
