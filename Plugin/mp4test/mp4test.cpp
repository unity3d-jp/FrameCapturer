#define MP4V2_USE_STATIC_LIB
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <windows.h>
#include "mp4v2/mp4v2.h"
#include "openh264/codec_api.h"
#include "fcH264Encoder.h"

#pragma comment(lib, "libmp4v2.lib")



#define OutputFile "test.mp4"
#define Width  320
#define Height 240


void CreateTestData(bRGBA *rgba, int width, int height, int scroll)
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


// mux raw h264 to mp4:
//  ffmpeg -f h264 -i in.264 -c:v copy out.mp4

int main(int argc, char** argv)
{
    fcH264Encoder encoder(Width, Height, 30.0f, 128000);
    if (!encoder) {
        printf("Failed to create H264 encoder.\n");
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

    std::vector<bRGBA> pic_rgba(Width * Height);

    std::ofstream fo("test.h264", std::ios::binary);
    for (int i = 0; i < 100; ++i) {
        CreateTestData(&pic_rgba[0], Width, Height, i);
        auto r = encoder.encodeRGBA(&pic_rgba[0]);
        if (r.data) {
            MP4WriteSample(mp4, videoTrackId, (uint8_t*)r.data, r.data_size);
            fo.write((char*)r.data, r.data_size);
        }
    }


    MP4Dump(mp4);
    MP4Close(mp4);
}

