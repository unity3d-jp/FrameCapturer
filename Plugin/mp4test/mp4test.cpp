#define MP4V2_USE_STATIC_LIB
#include "mp4v2/mp4v2.h"
#include "openh264/codec_api.h"
#include <cstdint>
#include <cstdlib>
#include <windows.h>

#pragma comment(lib, "libmp4v2.lib")

#if defined(_M_AMD64)
    #define OpenH264DLL "openh264-1.4.0-win64msvc.dll"
#elif defined(_M_IX86)
    #define OpenH264DLL "openh264-1.4.0-win32msvc.dll"
#endif
typedef int  (*WelsCreateSVCEncoderT)(ISVCEncoder** ppEncoder);
typedef void (*WelsDestroySVCEncoderT)(ISVCEncoder* pEncoder);


#define OutputFile "test.mp4"
#define Width  320
#define Height 240

union RGBA
{
    struct { uint8_t r, g, b, a; };
    uint8_t v[4];
};


static inline int RGBToY(uint8_t r, uint8_t g, uint8_t b)
{
    return (66 * r + 129 * g + 25 * b + 0x1080) >> 8;
}
static __inline int RGBToU(uint8_t r, uint8_t g, uint8_t b)
{
    return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8_t r, uint8_t g, uint8_t b)
{
    return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

static inline void RGBAToYRow(const RGBA* src, uint8_t* dst_y, int width)
{
    for (int x = 0; x < width; ++x) {
        dst_y[0] = RGBToY(src[0].r, src[0].g, src[0].b);
        src += 1;
        dst_y += 1;
    }
}

void RGBAToUVRow(const RGBA* src, uint8_t* dst_u, uint8_t* dst_v, int width)
{
    const RGBA* src1 = src + width;
    int x;
    for (x = 0; x < width - 1; x += 2) {
        uint8_t ab = (src[0].b + src[1].b + src1[0].b + src1[1].b) >> 2;
        uint8_t ag = (src[0].g + src[1].g + src1[0].g + src1[1].g) >> 2;
        uint8_t ar = (src[0].r + src[1].r + src1[0].r + src1[1].r) >> 2;
        dst_u[0] = RGBToU(ar, ag, ab);
        dst_v[0] = RGBToV(ar, ag, ab);
        src += 2;
        src1 += 2;
        dst_u += 1;
        dst_v += 1;
    }
    if (width & 1) {
        uint8_t ab = (src[0].b + src1[1].b) >> 1;
        uint8_t ag = (src[0].g + src1[1].g) >> 1;
        uint8_t ar = (src[0].r + src1[1].r) >> 1;
        dst_u[0] = RGBToU(ar, ag, ab);
        dst_v[0] = RGBToV(ar, ag, ab);
    }
}

#define SUBSAMPLE(v, a) ((((v) + (a) - 1)) / (a))

static void RGBAToI420(uint8_t* dst_y, uint8_t *dst_u, uint8_t *dst_v, const RGBA *src, int width, int height)
{
    int stride_y = width;
    int stride_u = width * 12 / 8;
    int stride_v = width * 12 / 8;
    for (int y = 0; y < height - 1; y += 2) {
        RGBAToUVRow(src, dst_u, dst_v, width);
        RGBAToYRow(src, dst_y, width);
        RGBAToYRow(src + width, dst_y + width, width);
        src += width * 2;
        dst_y += stride_y * 2;
        dst_u += stride_u;
        dst_v += stride_v;
    }
    if (height & 1) {
        RGBAToUVRow(src, dst_u, dst_v, width);
        RGBAToYRow(src, dst_y, width);
    }
}


void CreateTestData(RGBA *rgba, int width, int height, int scroll)
{
    const int block_size = 32;
    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            int ip = iy * width + ix;
            int yb = iy / block_size;
            int xb = (ix + scroll) / block_size;

            if ((xb+yb)%2==0) {
                rgba[ip].r = rgba[ip].g = rgba[ip].b = 255;
            }
            else {
                rgba[ip].r = rgba[ip].g = rgba[ip].b = 0;
            }

            //if ((xb + yb) % 2 == 0)  {
            //    rgba[ip].r += 128;
            //}
            //if ((xb + yb) % 3 == 0)  {
            //    rgba[ip].g += 128;
            //}
            //if ((xb + yb) % 5 == 0)  {
            //    rgba[ip].b += 128;
            //}
            ip++;
        }
    }
}


int main(int argc, char** argv)
{
    HMODULE h264_dll = nullptr;
    WelsCreateSVCEncoderT pWelsCreateSVCEncoder = nullptr;
    WelsDestroySVCEncoderT pWelsDestroySVCEncoder = nullptr;

    ISVCEncoder *h264_encoder = nullptr;
    {
        h264_dll = ::LoadLibraryA(OpenH264DLL);
        if (h264_dll != nullptr) {
            pWelsCreateSVCEncoder = (WelsCreateSVCEncoderT)::GetProcAddress(h264_dll, "WelsCreateSVCEncoder");
            pWelsDestroySVCEncoder = (WelsDestroySVCEncoderT)::GetProcAddress(h264_dll, "WelsDestroySVCEncoder");
            if (pWelsCreateSVCEncoder != nullptr)
            {
                pWelsCreateSVCEncoder(&h264_encoder);
            }
        }
    }
    if (h264_encoder == nullptr) {
        printf("Failed to create H264 encoder.\n");
        return 0;
    }

    {
        int trace_level = WELS_LOG_DEBUG;
        h264_encoder->SetOption(ENCODER_OPTION_TRACE_LEVEL, &trace_level);

        SEncParamBase param;
        memset(&param, 0, sizeof(SEncParamBase));
        param.iUsageType = CAMERA_VIDEO_REAL_TIME;
        param.fMaxFrameRate = 30.0f;
        param.iPicWidth = Width;
        param.iPicHeight = Height;
        param.iTargetBitrate = 256000;
        int ret = h264_encoder->Initialize(&param);

        printf("Initialize(): %d\n", ret);
        int videoFormat = videoFormatI420;
        h264_encoder->SetOption(ENCODER_OPTION_DATAFORMAT, &videoFormat);

    }


    uint32_t verbosity = 0;

    MP4FileHandle mp4 = MP4Create(OutputFile, verbosity);
    if (!mp4) {
        printf("MP4Create() failed\n");
        return 0;
    }

    printf("Created skeleton\n");
    MP4Dump(mp4);

    MP4SetODProfileLevel(mp4, 1);
    MP4SetSceneProfileLevel(mp4, 1);
    MP4SetVideoProfileLevel(mp4, 1);
    MP4SetAudioProfileLevel(mp4, 1);
    MP4SetGraphicsProfileLevel(mp4, 1);

    MP4TrackId videoTrackId = MP4AddH264VideoTrack(mp4, 90000, 3000, Width, Height, 1, 2, 3, 1);
    //MP4TrackId odTrackId = MP4AddODTrack(mp4);
    //MP4TrackId bifsTrackId =  MP4AddSceneTrack(mp4);
    //MP4TrackId videoHintTrackId = MP4AddHintTrack(mp4, videoTrackId);
    //MP4TrackId audioTrackId = MP4AddAudioTrack(mp4, 44100, 1152);

    //static uint8_t pseq[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    //MP4AddH264SequenceParameterSet(mp4, videoTrackId, pseq, 10);
    //MP4AddH264SequenceParameterSet(mp4, videoTrackId, pseq, 6);
    //MP4AddH264PictureParameterSet(mp4, videoTrackId, pseq, 7);
    //MP4AddH264PictureParameterSet(mp4, videoTrackId, pseq, 8);
    //MP4AddH264PictureParameterSet(mp4, videoTrackId, pseq, 7);

    RGBA *data_rgba = new RGBA[Width * Height];
    uint8_t *data_yuv = new uint8_t[Width * Height * 2];
    uint8_t *data_y = data_yuv;
    uint8_t *data_u = data_y + (Width * Height);
    uint8_t *data_v = data_u + (Width * Height >> 2);
    for (int i = 0; i < 100; ++i) {
        // make test data
        CreateTestData(data_rgba, Width, Height, i);
        RGBAToI420(data_y, data_u, data_v, data_rgba, Width, Height);

        SSourcePicture src;
        memset(&src, 0, sizeof(SSourcePicture));
        src.iPicWidth = Width;
        src.iPicHeight = Height;
        src.iColorFormat = videoFormatI420;
        src.iStride[0] = src.iPicWidth;
        src.iStride[1] = src.iStride[2] = src.iPicWidth >> 2;
        src.pData[0] = data_y;
        src.pData[1] = data_u;
        src.pData[2] = data_v;

        SFrameBSInfo dst;
        memset(&dst, 0, sizeof(SFrameBSInfo));

        int ret = h264_encoder->EncodeFrame(&src, &dst);
        if (ret!=0) {
            printf("EncodeFrame() failed\n");
            break;
        }
        if (dst.eFrameType == videoFrameTypeSkip) {
            continue;
        }
        MP4WriteSample(mp4, videoTrackId, dst.sLayerInfo[0].pBsBuf, dst.iFrameSizeInBytes);
    }
    delete[] data_yuv;
    delete[] data_rgba;


    MP4Dump(mp4);
    MP4Close(mp4);

    // mp4 header size: 2725

    if (pWelsDestroySVCEncoder != nullptr)
    {
        pWelsDestroySVCEncoder(h264_encoder);
    }
}

