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
#define SamplingRate 48000

struct RGBA
{
    uint8_t r, g, b, a;
};

void CreateTestVideoData(RGBA *rgba, int width, int height, int scroll)
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
void CreateTestAudioData(float *samples, int num_samples, int scroll)
{
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = std::sin((float(i) * 2.5f) * (3.14159f / 180.0f)) * 32767.0f;
    }
}



int main(int argc, char** argv)
{
    fcMP4Config conf;
    conf.video = true;
    conf.video_width = Width;
    conf.video_height = Height;
    conf.video_bitrate = 256000;
    conf.video_framerate = 30;
    conf.audio = true;
    conf.audio_sampling_rate = SamplingRate;
    conf.audio_num_channels = 1;
    conf.audio_bitrate = 64000;
    fcIMP4Context *ctx = fcMP4CreateContext(&conf);

    std::vector<RGBA> video_frame(Width * Height);
    std::vector<float> audio_sample(SamplingRate);
    for (int i = 0; i < 300; ++i) {
        CreateTestVideoData(&video_frame[0], Width, Height, i);
        fcMP4AddVideoFramePixels(ctx, &video_frame[0]);
    }
    for (int i = 0; i < 600; ++i) {
        CreateTestAudioData(&audio_sample[0], audio_sample.size(), 0);
        fcMP4AddAudioSamples(ctx, &audio_sample[0], audio_sample.size());
    }
    fcMP4WriteFile(ctx, "out.mp4", 0, -1);

    fcMP4DestroyContext(ctx);
}

