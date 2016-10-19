#include "TestCommon.h"
using namespace std::literals;


// custom stream functions (just a wrapper of FILE)
static size_t tellp(void *f) { return ftell((FILE*)f); }
static void   seekp(void *f, size_t pos) { fseek((FILE*)f, (long)pos, SEEK_SET); }
static size_t write(void *f, const void *data, size_t len) { return fwrite(data, 1, len, (FILE*)f); }


void WebMTest()
{
    printf("WebMTest begin\n");


    const int DurationInSeconds = 10;
    const int FrameRate = 60;
    const int Width = 320;
    const int Height = 240;
    const int SamplingRate = 48000;

    fcWebMConfig conf;
    conf.video_width = Width;
    conf.video_height = Height;
    conf.video_bitrate = 256000;
    conf.audio_sample_rate = SamplingRate;
    conf.audio_num_channels = 1;
    conf.audio_bitrate = 64000;


    // create output streams
    fcStream* fstream = fcCreateFileStream("file_stream.webm");
    fcStream* mstream = fcCreateMemoryStream();
    FILE *ofile = fopen("custom_stream.webm", "wb");
    fcStream* cstream = fcCreateCustomStream(ofile, &tellp, &seekp, &write);

    // create mp4 context and add output streams
    fcIWebMContext *ctx = fcWebMCreateContext(&conf);
    fcWebMAddOutputStream(ctx, fstream);
    fcWebMAddOutputStream(ctx, mstream);
    fcWebMAddOutputStream(ctx, cstream);

    // create movie data
    {
        // add video frames
        std::thread video_thread = std::thread([&]() {
            RawVector<RGBAu8> video_frame(Width * Height);
            fcTime t = 0;
            for (int i = 0; i < DurationInSeconds * FrameRate; ++i) {
                CreateVideoData(&video_frame[0], Width, Height, i);
                fcWebMAddVideoFramePixels(ctx, &video_frame[0], fcPixelFormat_RGBAu8, t);
                t += 1.0 / FrameRate;
            }
        });

        // add audio frames
        std::thread audio_thread = std::thread([&]() {
            RawVector<float> audio_sample(SamplingRate);
            fcTime t = 0;
            for (int i = 0; i < DurationInSeconds; ++i) {
                CreateAudioData(&audio_sample[0], (int)audio_sample.size(), i);
                fcWebMAddAudioFrame(ctx, &audio_sample[0], (int)audio_sample.size(), t);
                t += 1.0;
            }
        });

        // wait
        video_thread.join();
        audio_thread.join();
    }

    // destroy mp4 context
    fcWebMDestroyContext(ctx);

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

    printf("WebMTest end\n");
}

