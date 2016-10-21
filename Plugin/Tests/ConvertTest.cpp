#include "TestCommon.h"

const void* fcConvertPixelFormat(void *dst, fcPixelFormat dstfmt, const void *src, fcPixelFormat srcfmt, size_t size);

const int Width = 320;
const int Height = 240;


template<class Src, class Dst>
void ConvertTestImpl(fcIPngContext *ctx, RawVector<Src>& src)
{
    char filename[128];
    sprintf(filename, "%s_to_%s.png", GetPixelFormat<Src>::getName(), GetPixelFormat<Dst>::getName());

    RawVector<Dst> dst;
    dst.resize(Width * Height);
    auto data = fcConvertPixelFormat(&dst[0], GetPixelFormat<Dst>::value, &src[0], GetPixelFormat<Src>::value, src.size());
    fcPngExportPixels(ctx, filename, data, Width, Height, GetPixelFormat<Dst>::value);
}

void ConvertTest()
{
    printf("ConvertTest begin\n");

    fcPngConfig conf;
    fcIPngContext *ctx = fcPngCreateContext(&conf);

#define TestCases(SrcT)\
{\
    RawVector<SrcT> video_frame(Width * Height);\
    CreateVideoData(&video_frame[0], Width, Height, 0);\
    ConvertTestImpl<SrcT, RGBAu8>(ctx, video_frame);\
    ConvertTestImpl<SrcT, RGBu8>(ctx, video_frame);\
    ConvertTestImpl<SrcT, RGu8>(ctx, video_frame);\
    ConvertTestImpl<SrcT, Ru8>(ctx, video_frame);\
    ConvertTestImpl<SrcT, RGBAf16>(ctx, video_frame);\
    ConvertTestImpl<SrcT, RGBf16>(ctx, video_frame);\
    ConvertTestImpl<SrcT, RGf16>(ctx, video_frame);\
    ConvertTestImpl<SrcT, Rf16>(ctx, video_frame);\
    ConvertTestImpl<SrcT, RGBAf32>(ctx, video_frame);\
    ConvertTestImpl<SrcT, RGBf32>(ctx, video_frame);\
    ConvertTestImpl<SrcT, RGf32>(ctx, video_frame);\
    ConvertTestImpl<SrcT, Rf32>(ctx, video_frame);\
}

    TestCases(RGBAu8);
    TestCases(RGBu8);
    TestCases(RGu8);
    TestCases(Ru8);
    TestCases(RGBAf16);
    TestCases(RGBf16);
    TestCases(RGf16);
    TestCases(Rf16);
    TestCases(RGBAf32);
    TestCases(RGBf32);
    TestCases(RGf32);
    TestCases(Rf32);

    fcPngDestroyContext(ctx);

    printf("ConvertTest end\n");

}
