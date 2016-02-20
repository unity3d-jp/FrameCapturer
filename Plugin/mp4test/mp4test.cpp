#ifdef _MSC_VER
    #pragma warning(disable: 4190)
    #define _CRT_SECURE_NO_WARNINGS
#endif
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <thread>
#include <windows.h>
#include "../FrameCapturer.h"


// video data generator

struct RGBA { uint8_t r, g, b, a; };

void CreateVideoData(RGBA *rgba, int width, int height, int scroll)
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

// audio data generator

void CreateAudioData(float *samples, int num_samples, int scroll)
{
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = std::sin((float(i + (num_samples * scroll)) * 0.5f) * (3.14159f / 180.0f)) * 32767.0f;
    }
}

// custom stream functions
size_t tellp(void *f) { return ftell((FILE*)f); }
void   seekp(void *f, size_t pos) { fseek((FILE*)f, (long)pos, SEEK_SET); }
size_t write(void *f, const void *data, size_t len) { return fwrite(data, 1, len, (FILE*)f); }


int main(int argc, char** argv)
{
    using namespace std::literals;

    // download OpenH264 codec
    fcMP4DownloadCodecBegin();
    for (int i = 0; i < 10; ++i) {
        if (fcMP4DownloadCodecGetState() == fcDownloadState_InProgress) {
            std::this_thread::sleep_for(1s);
        }
        else { break; }
    }

    const int DurationInSeconds = 10;
    const int FrameRate = 60;
    const int Width = 320;
    const int Height = 240;
    const int SamplingRate = 48000;

    fcMP4Config conf;
    conf.video_width = Width;
    conf.video_height = Height;
    conf.video_bitrate = 256000;
    conf.audio_sample_rate = SamplingRate;
    conf.audio_num_channels = 1;
    conf.audio_bitrate = 64000;


    // create output streams
    fcStream* fstream = fcCreateFileStream("file_stream.mp4");
    fcStream* mstream = fcCreateMemoryStream();
    FILE *ofile = fopen("custom_stream.mp4", "wb");
    fcStream* cstream = fcCreateCustomStream(ofile, &tellp, &seekp, &write);

    // create mp4 context and add output streams
    fcIMP4Context *ctx = fcMP4CreateContext(&conf);
    fcMP4AddOutputStream(ctx, fstream);
    fcMP4AddOutputStream(ctx, mstream);
    fcMP4AddOutputStream(ctx, cstream);

    // add video data
    std::thread video_thread = std::thread([&]() {
        std::vector<RGBA> video_frame(Width * Height);
        fcTime t = 0;
        for (int i = 0; i < DurationInSeconds * FrameRate; ++i) {
            CreateVideoData(&video_frame[0], Width, Height, i);
            fcMP4AddVideoFramePixels(ctx, &video_frame[0], fcColorSpace_RGBA, t);
            t += 1000000000LLU / FrameRate;
        }
    });

    // add audio data
    std::thread audio_thread = std::thread([&]() {
        std::vector<float> audio_sample(SamplingRate);
        fcTime t = 0;
        for (int i = 0; i < DurationInSeconds; ++i) {
            CreateAudioData(&audio_sample[0], (int)audio_sample.size(), i);
            fcMP4AddAudioFrame(ctx, &audio_sample[0], (int)audio_sample.size(), t);
            t += 1000000000LLU;
        }
    });

    // wait
    video_thread.join();
    audio_thread.join();

    // destroy mp4 context
    fcMP4DestroyContext(ctx);

    // destroy output streams
    {
        fcBufferData bd = fcStreamGetBufferData(mstream);
        std::fstream of("memory_stream.mp4", std::ios::binary | std::ios::out);
        of.write((char*)bd.data, bd.size);
    }
    fcDestroyStream(fstream);
    fcDestroyStream(mstream);
    fcDestroyStream(cstream);
    fclose(ofile);
}

