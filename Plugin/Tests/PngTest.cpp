#include "TestCommon.h"

int main(int argc, char** argv)
{
    const int Width = 320;
    const int Height = 240;

    fcPngConfig conf;
    fcIPngContext *ctx = fcPngCreateContext(&conf);

    {
        std::vector<RGB> video_frame(Width * Height);
        CreateVideoData(&video_frame[0], Width, Height, 0);
        fcPngExportPixels(ctx, "RGB8.png", &video_frame[0], Width, Height, fcPixelFormat_RGB8);
    }
    {
        std::vector<hRGB> video_frame(Width * Height);
        CreateVideoData(&video_frame[0], Width, Height, 0);
        fcPngExportPixels(ctx, "RGB16.png", &video_frame[0], Width, Height, fcPixelFormat_RGBHalf);
    }
    {
        std::vector<fRGB> video_frame(Width * Height);
        CreateVideoData(&video_frame[0], Width, Height, 0);
        fcPngExportPixels(ctx, "RGB32to16.png", &video_frame[0], Width, Height, fcPixelFormat_RGBFloat);
    }

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
        fcPngExportPixels(ctx, "RGBA32to16.png", &video_frame[0], Width, Height, fcPixelFormat_RGBAFloat);
    }

    fcPngDestroyContext(ctx);
}

