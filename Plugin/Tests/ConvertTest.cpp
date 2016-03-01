#include "TestCommon.h"

void fcConvert(void *dst, size_t dstsize, fcPixelFormat dstfmt, const void *src, size_t srcsize, fcPixelFormat srcfmt);

const int Width = 320;
const int Height = 240;


template<class Dst, class Src>
void ConvertTestImpl(fcIPngContext *ctx, std::vector<Src>& src, const char *filename)
{
    std::vector<Dst> dst;
    dst.resize(Width * Height);
    fcConvert(&dst[0], GetPixelFormat<Dst>::value, &src[0], GetPixelFormat<Src>::value, src.size());
    fcPngExportPixels(ctx, filename, &dst[0], Width, Height, GetPixelFormat<Dst>::value);
}

void ConvertTest()
{
    printf("ConvertTest begin\n");

    fcPngConfig conf;
    fcIPngContext *ctx = fcPngCreateContext(&conf);

    std::vector<RGBAu8> video_frame(Width * Height);
    CreateVideoData(&video_frame[0], Width, Height, 0);

    fcPngDestroyContext(ctx);

    printf("ConvertTest end\n");

}
