#include "TestCommon.h"

int main(int argc, char** argv)
{
    const int Width = 320;
    const int Height = 240;

    fcPngConfig conf;
    fcIPngContext *ctx = fcPngCreateContext(&conf);

    {
        std::vector<RGBA> video_frame(Width * Height);
        CreateVideoData(&video_frame[0], Width, Height, 0);
        fcPngExportPixels(ctx, "RGBA8.png", &video_frame[0], Width, Height, fcPixelFormat_RGBA8);
    }
    {
        std::vector<hRGBA> video_frame(Width * Height);
        CreateVideoData(&video_frame[0], Width, Height, 0);
        fcPngExportPixels(ctx, "RGBA16.png", &video_frame[0], Width, Height, fcPixelFormat_RGBAHalf);
    }
    {
        std::vector<fRGBA> video_frame(Width * Height);
        CreateVideoData(&video_frame[0], Width, Height, 0);
        fcPngExportPixels(ctx, "RGBA32.png", &video_frame[0], Width, Height, fcPixelFormat_RGBAFloat);
    }

    fcPngDestroyContext(ctx);
}

