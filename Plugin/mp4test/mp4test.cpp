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
static inline int RGBToU(uint8_t r, uint8_t g, uint8_t b)
{
    return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static inline int RGBToV(uint8_t r, uint8_t g, uint8_t b)
{
    return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

static inline void RGBAToYRow(uint8_t* dst_y, const RGBA* src, int width)
{
    for (int x = 0; x < width; ++x) {
        dst_y[0] = RGBToY(src[0].r, src[0].g, src[0].b);
        src += 1;
        dst_y += 1;
    }
}

static void RGBAToUVRow(uint8_t* dst_u, uint8_t* dst_v, const RGBA* src, int width)
{
    const RGBA* src1 = src + width;
    for (int x = 0; x < width - 1; x += 2) {
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
    //if (width & 1) {
    //    uint8_t ab = (src[0].b + src1[1].b) >> 1;
    //    uint8_t ag = (src[0].g + src1[1].g) >> 1;
    //    uint8_t ar = (src[0].r + src1[1].r) >> 1;
    //    dst_u[0] = RGBToU(ar, ag, ab);
    //    dst_v[0] = RGBToV(ar, ag, ab);
    //    dst_u += 1;
    //    dst_v += 1;
    //}
}

static void RGBAToI420(uint8_t* dst_y, uint8_t *dst_u, uint8_t *dst_v, const RGBA *src, int width, int height)
{
    int stride_uv = width / 2;
    //int stride_uv = width * 1;
    //int stride_uv = 0;
    for (int y = 0; y < height - 1; y += 2) {
        RGBAToUVRow(dst_u, dst_v, src, width);
        RGBAToYRow(dst_y, src, width);
        RGBAToYRow(dst_y + width, src + width, width);
        src += width * 2;
        dst_y += width * 2;
        dst_u += stride_uv;
        dst_v += stride_uv;
    }
    //if (height & 1) {
    //    RGBAToUVRow(dst_u, dst_v, src, width);
    //    RGBAToYRow(dst_y, src, width);
    //}
}


void CreateTestData(RGBA *rgba, int width, int height, int scroll)
{
    const int block_size = 32;
    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            int ip = iy * width + ix;
            int yb = iy / block_size;
            int xb = (ix + iy + scroll) / block_size;

            if ((xb)%2==0) {
                rgba[ip].r = rgba[ip].g = rgba[ip].b = rgba[ip].a = 255;
            }
            else {
                rgba[ip].r = rgba[ip].g = rgba[ip].b = rgba[ip].a = 0;
            }
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
        param.iTargetBitrate = 128000;
        param.iRCMode = RC_BITRATE_MODE;
        int ret = h264_encoder->Initialize(&param);

        printf("Initialize(): %d\n", ret);
    }


    MP4FileHandle mp4 = MP4Create(OutputFile, 0);
    if (!mp4) {
        printf("MP4Create() failed\n");
        DebugBreak();
        return 0;
    }
    MP4SetTimeScale(mp4, 90000);
    MP4SetVideoProfileLevel(mp4, 0x7F);

    printf("Created skeleton\n");
    MP4Dump(mp4);

    //MP4SetODProfileLevel(mp4, 1);
    //MP4SetSceneProfileLevel(mp4, 1);
    //MP4SetVideoProfileLevel(mp4, 1);
    //MP4SetAudioProfileLevel(mp4, 1);
    //MP4SetGraphicsProfileLevel(mp4, 1);

    MP4TrackId videoTrackId = MP4AddH264VideoTrack(mp4, 90000, MP4_INVALID_DURATION, Width, Height, 1, 2, 3, 1);
    //MP4TrackId odTrackId = MP4AddODTrack(mp4);
    //MP4TrackId bifsTrackId =  MP4AddSceneTrack(mp4);
    //MP4TrackId videoHintTrackId = MP4AddHintTrack(mp4, videoTrackId);
    //MP4TrackId audioTrackId = MP4AddAudioTrack(mp4, 44100, 1152);
    //MP4TrackId audioHintTrackId = MP4AddHintTrack(mp4, audioTrackId);

    //static uint8_t pseq[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    //MP4AddH264SequenceParameterSet(mp4, videoTrackId, pseq, 10);
    //MP4AddH264SequenceParameterSet(mp4, videoTrackId, pseq, 6);
    //MP4AddH264PictureParameterSet(mp4, videoTrackId, pseq, 7);
    //MP4AddH264PictureParameterSet(mp4, videoTrackId, pseq, 8);
    //MP4AddH264PictureParameterSet(mp4, videoTrackId, pseq, 7);

    uint8_t sps[] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2 };
    uint8_t pps[] = { 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
    int sps_len = sizeof(sps);
    int pps_len = sizeof(pps);
    MP4AddH264SequenceParameterSet(mp4, videoTrackId, sps, sps_len);
    MP4AddH264PictureParameterSet(mp4, videoTrackId, pps, pps_len);

    RGBA *pic_rgba = new RGBA[Width * Height];
    uint8_t *pic_yuv = new uint8_t[Width * Height * 3 / 2];
    uint8_t *pic_y = pic_yuv;
    uint8_t *pic_u = pic_y + (Width * Height);
    uint8_t *pic_v = pic_u + ((Width * Height) >> 2);
    for (int i = 0; i < 100; ++i) {
        CreateTestData(pic_rgba, Width, Height, i);
        RGBAToI420(pic_y, pic_u, pic_v, pic_rgba, Width, Height);

        SSourcePicture src;
        memset(&src, 0, sizeof(src));
        src.iPicWidth = Width;
        src.iPicHeight = Height;
        src.iColorFormat = videoFormatI420;
        src.pData[0] = pic_y;
        src.pData[1] = pic_u;
        src.pData[2] = pic_v;
        src.iStride[0] = Width;
        src.iStride[1] = Width >> 1;
        src.iStride[2] = Width >> 1;

        SFrameBSInfo dst;
        memset(&dst, 0, sizeof(dst));

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
    delete[] pic_yuv;
    delete[] pic_rgba;


    MP4Dump(mp4);
    MP4Close(mp4);

    if (pWelsDestroySVCEncoder != nullptr)
    {
        pWelsDestroySVCEncoder(h264_encoder);
    }
}

