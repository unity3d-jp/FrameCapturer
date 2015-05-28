#include "pch.h"
#include "FrameCapturer.h"


class fcExrContext
{
public:
private:
    std::string m_path;
};


fcCLinkage fcExport fcExrContext* fcExrCreateFile(const char *path, int width, int height, fcETextureFormat fmt)
{
    return nullptr;
}

fcCLinkage fcExport void fcExrCloseFile(fcExrContext *ctx)
{

}

fcCLinkage fcExport void fcExrWriteFrame(fcExrContext *ctx, void *tex)
{

}

fcCLinkage fcExport void fcExrBeginWriteFrame(fcExrContext *ctx, void *tex)
{

}

fcCLinkage fcExport void fcExrEndWriteFrame(fcExrContext *ctx)
{

}
