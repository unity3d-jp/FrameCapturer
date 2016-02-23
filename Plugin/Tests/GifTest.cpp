#include "TestCommon.h"

template<class T>
void Test(const char *filename)
{
    const int Width = 320;
    const int Height = 240;

    fcGifConfig conf;
    conf.width = Width;
    conf.height = Height;
    fcIGifContext *ctx = fcGifCreateContext(&conf);

    std::vector<T> video_frame(Width * Height);
    for (int i = 0; i < 15; ++i) {
        CreateVideoData(&video_frame[0], Width, Height, i);
        fcGifAddFramePixels(ctx, &video_frame[0], GetPixelFormat<T>::value);
    }
    fcGifWriteFile(ctx, filename);
    fcGifDestroyContext(ctx);
}

int main(int argc, char** argv)
{
    Test<RGB>("RGB.gif");
    Test<hRGB>("hRGB.gif");
    Test<fRGB>("fRGB.gif");
    Test<RGBA>("RGBA.gif");
    Test<hRGBA>("hRGBA.gif");
    Test<fRGBA>("fRGBA.gif");
}

