#include "TestCommon.h"

template<class T>
void GifTestImpl(const char *filename)
{
    const int Width = 320;
    const int Height = 240;
    const int frame_count = 30;

    fcGifConfig conf;
    conf.width = Width;
    conf.height = Height;
    fcIGifContext *ctx = fcGifCreateContext(&conf);

    fcTime t = 0;
    RawVector<T> video_frame(Width * Height);
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

    fcTaskGroup group;
    group.run([]() { GifTestImpl<RGBu8>("RGBu8.gif"); });
    group.run([]() { GifTestImpl<RGBf16>("RGBf16.gif"); });
    group.run([]() { GifTestImpl<RGBf32>("RGBf32.gif"); });
    group.run([]() { GifTestImpl<RGBAu8>("RGBAu8.gif"); });
    group.run([]() { GifTestImpl<RGBAf16>("RGBAf16.gif"); });
    group.run([]() { GifTestImpl<RGBAf32>("RGBAf32.gif"); });
    group.wait();

    printf("GifTest end\n");
}

