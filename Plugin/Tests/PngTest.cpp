#include "TestCommon.h"

template<class T>
void PngTestImpl(fcIPngContext *ctx, const char *filename)
{
    const int Width = 320;
    const int Height = 240;

    std::vector<T> video_frame(Width * Height);
    CreateVideoData(&video_frame[0], Width, Height, 0);
    fcPngExportPixels(ctx, filename, &video_frame[0], Width, Height, GetPixelFormat<T>::value);
}

void PngTest()
{
    fcPngConfig conf;
    fcIPngContext *ctx = fcPngCreateContext(&conf);

    PngTestImpl<RGB>(ctx, "RGB.png");
    PngTestImpl<hRGB>(ctx, "hRGB.png");
    PngTestImpl<fRGB>(ctx, "fRGB.png");
    PngTestImpl<RGBA>(ctx, "RGBA.png");
    PngTestImpl<hRGBA>(ctx, "hRGBA.png");
    PngTestImpl<fRGBA>(ctx, "fRGBA.png");

    fcPngDestroyContext(ctx);
}

