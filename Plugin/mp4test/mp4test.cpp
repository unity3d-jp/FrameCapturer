#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <thread>
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
        samples[i] = std::sin((float(i + (num_samples * scroll)) * 0.5f) * (3.14159f / 180.0f)) * 32767.0f;
    }
}


bool g_download_completed = false;

void DownloadCallback(bool is_complete, const char *status)
{
    puts(status);
    g_download_completed = is_complete;
}

int main(int argc, char** argv)
{
    using namespace std::literals;
    fcMP4DownloadCodec(DownloadCallback);
    for (int i = 0; i < 10; ++i) {
        if (g_download_completed) { break; }
        std::this_thread::sleep_for(1s);
    }

    fcMP4Config conf;
    conf.video = true;
    conf.video_width = Width;
    conf.video_height = Height;
    conf.video_bitrate = 256000;
    conf.video_framerate = 30;
    conf.audio = true;
    conf.audio_sample_rate = SamplingRate;
    conf.audio_num_channels = 1;
    conf.audio_bitrate = 64000;

    fcStream* fs0 = fcCreateFileStream("test0.mp4");
    fcIMP4Context *ctx = fcMP4CreateContext(&conf);
    fcMP4AddStream(ctx, fs0);


    std::thread video_thread = std::thread([&]() {
        std::vector<RGBA> video_frame(Width * Height);
        for (int i = 0; i < 300; ++i) {
            CreateTestVideoData(&video_frame[0], Width, Height, i);
            fcMP4AddVideoFramePixels(ctx, &video_frame[0]);

            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / conf.video_framerate));
        }
    });
    std::thread audio_thread = std::thread([&]() {
        std::vector<float> audio_sample(SamplingRate);
        for (int i = 0; i < 10; ++i) {
            CreateTestAudioData(&audio_sample[0], audio_sample.size(), i);
            fcMP4AddAudioSamples(ctx, &audio_sample[0], audio_sample.size());

            std::this_thread::sleep_for(1s);
        }
    });
    video_thread.join();
    audio_thread.join();

    fcMP4WriteFile(ctx, "out.mp4", 0, -1);

    fcMP4DestroyContext(ctx);
    fcDestroyStream(fs0);
}

