#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <windows.h>
#include "../FrameCapturer.h"


#define OutputH264 "test.h264"
#define OutputMP4 "test.mp4"
#define Width  320
#define Height 240

struct RGBA
{
    uint8_t r, g, b, a;
};

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
    fcMP4Config conf;
    conf.width = Width;
    conf.height = Height;
    conf.bitrate = 256000;
    conf.framerate = 30;
    fcIMP4Context *ctx = fcMP4CreateContext(&conf);

    std::vector<RGBA> pic_rgba(Width * Height);
    for (int i = 0; i < 100; ++i) {
        CreateTestData(&pic_rgba[0], Width, Height, i);
        fcMP4AddFramePixels(ctx, &pic_rgba[0]);
    }
    fcMP4WriteFile(ctx, "out.mp4", 0, -1);

    fcMP4DestroyContext(ctx);
}

