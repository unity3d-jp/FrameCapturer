#include "TestCommon.h"

template<class T>
void GifTestImpl(const char *filename)
{
    const int Width = 320;
    const int Height = 240;
    const int frame_count = 10;

    fcGifConfig conf;
    conf.width = Width;
    conf.height = Height;
    fcIGifContext *ctx = fcGifCreateContext(&conf);

    fcTime t = 0;
    std::vector<T> video_frame(Width * Height);
    for (int i = 0; i < frame_count; ++i) {
        CreateVideoData(&video_frame[0], Width, Height, i);
        fcGifAddFramePixels(ctx, &video_frame[0], GetPixelFormat<T>::value, false, t);
        t += 1.0 / 30.0;
    }

    fcStream *fstream = fcCreateFileStream(filename);
    fcGifWrite(ctx, fstream);
    fcDestroyStream(fstream);
    fcGifDestroyContext(ctx);
}

void GifTest()
{
    printf("GifTest begin\n");

    GifTestImpl<RGBu8>("RGBu8.gif");
    GifTestImpl<RGBf16>("RGBf16.gif");
    GifTestImpl<RGBf32>("RGBf32.gif");
    GifTestImpl<RGBAu8>("RGBAu8.gif");
    GifTestImpl<RGBAf16>("RGBAf16.gif");
    GifTestImpl<RGBAf32>("RGBAf32.gif");

    printf("GifTest end\n");
}

