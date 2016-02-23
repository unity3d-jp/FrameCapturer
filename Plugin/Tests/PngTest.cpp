#include "TestCommon.h"

template<class T>
void Test(fcIPngContext *ctx, const char *filename)
{
    const int Width = 320;
    const int Height = 240;

    std::vector<T> video_frame(Width * Height);
    CreateVideoData(&video_frame[0], Width, Height, 0);
    fcPngExportPixels(ctx, filename, &video_frame[0], Width, Height, GetPixelFormat<T>::value);
}

int main(int argc, char** argv)
{
    fcPngConfig conf;
    fcIPngContext *ctx = fcPngCreateContext(&conf);

    Test<RGB>(ctx, "RGB.png");
    Test<hRGB>(ctx, "hRGB.png");
    Test<fRGB>(ctx, "fRGB.png");
    Test<RGBA>(ctx, "RGBA.png");
    Test<hRGBA>(ctx, "hRGBA.png");
    Test<fRGBA>(ctx, "fRGBA.png");

    fcPngDestroyContext(ctx);
}

