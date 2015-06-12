#define MP4V2_USE_STATIC_LIB
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <windows.h>
#include "fcH264Encoder.h"
#include "fcMP4Muxer.h"


#define OutputH264 "test.h264"
#define OutputMP4 "test.mp4"
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



int main(int argc, char** argv)
{
    struct h264frame
    {
        std::string data;
        fcH264Encoder::FrameType type;
    };


    std::vector<h264frame> h264_frames;
    {
        fcH264Encoder encoder(Width, Height, 30.0f, 128000);
        if (!encoder) {
            printf("Failed to create H264 encoder.\n");
        }

        std::vector<bRGBA> pic_rgba(Width * Height);
        for (int i = 0; i < 100; ++i) {
            CreateTestData(&pic_rgba[0], Width, Height, i);
            auto r = encoder.encodeRGBA(&pic_rgba[0]);
            if (r.data) {
                h264_frames.push_back(h264frame());
                h264_frames.back().data.assign((char*)r.data, r.size);
                h264_frames.back().type = r.type;
            }
        }
    }

    // dump raw h264 frames to file
    {
        std::ofstream fo(OutputH264, std::ios::binary);
        for (const auto &frame : h264_frames) {
            fo.write((char*)&frame.data[0], frame.data.size());
        }
    }

    {
        fcMP4Muxer muxer;
        muxer.mux(OutputMP4, OutputH264, 30);
    }

}

