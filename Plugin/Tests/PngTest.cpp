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
    printf("PngTest begin\n");

    fcPngConfig conf;
    fcIPngContext *ctx = fcPngCreateContext(&conf);

    PngTestImpl<RGBu8>(ctx, "RGB.png");
    PngTestImpl<RGBf16>(ctx, "hRGB.png");
    PngTestImpl<RGBf32>(ctx, "fRGB.png");
    PngTestImpl<RGBAu8>(ctx, "RGBA.png");
    PngTestImpl<RGBAf16>(ctx, "hRGBA.png");
    PngTestImpl<RGBAf32>(ctx, "fRGBA.png");

    fcPngDestroyContext(ctx);

    printf("PngTest end\n");
}
