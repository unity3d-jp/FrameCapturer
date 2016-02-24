#include "TestCommon.h"

template<class T>
void GifTestImpl(const char *filename)
{
    const int Width = 320;
    const int Height = 240;

    fcGifConfig conf;
    conf.width = Width;
    conf.height = Height;
    fcIGifContext *ctx = fcGifCreateContext(&conf);

    fcTime t = 0;
    std::vector<T> video_frame(Width * Height);
    for (int i = 0; i < 15; ++i) {
        CreateVideoData(&video_frame[0], Width, Height, i);
        fcGifAddFramePixels(ctx, &video_frame[0], GetPixelFormat<T>::value, false, t);
        t += 1.0 / 30.0;
    }
    fcGifWriteFile(ctx, filename);
    fcGifDestroyContext(ctx);
}

void GifTest()
{
    GifTestImpl<RGB>("RGB.gif");
    GifTestImpl<hRGB>("hRGB.gif");
    GifTestImpl<fRGB>("fRGB.gif");
    GifTestImpl<RGBA>("RGBA.gif");
    GifTestImpl<hRGBA>("hRGBA.gif");
    GifTestImpl<fRGBA>("fRGBA.gif");
}

