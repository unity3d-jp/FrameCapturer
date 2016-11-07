#include "TestCommon.h"
using namespace std::literals;


// custom stream functions (just a wrapper of FILE)
static size_t tellp(void *f) { return ftell((FILE*)f); }
static void   seekp(void *f, size_t pos) { fseek((FILE*)f, (long)pos, SEEK_SET); }
static size_t write(void *f, const void *data, size_t len) { return fwrite(data, 1, len, (FILE*)f); }

void WebMTest(fcWebMVideoEncoder ve, fcWebMAudioEncoder ae)
{
    const int DurationInSeconds = 10;
    const int FrameRate = 60;
    const int Width = 320;
    const int Height = 240;
    const int SamplingRate = 48000;

    fcWebMConfig conf;
    //conf.video = false;
    conf.video_encoder = ve;
    conf.video_width = Width;
    conf.video_height = Height;
    //conf.video_bitrate_mode = fcVBR;
    conf.video_target_bitrate = 256 * 1000;
    conf.audio_encoder = ae;
    conf.audio_sample_rate = SamplingRate;
    conf.audio_num_channels = 1;
    conf.audio_target_bitrate = 64 * 1000;

    const char *video_encoder_name = nullptr;
    const char *audio_encoder_name = nullptr;
    switch (ve) {
    case fcWebMVideoEncoder::VP8: video_encoder_name = "VP8"; break;
    case fcWebMVideoEncoder::VP9: video_encoder_name = "VP9"; break;
    }
    switch (ae) {
    case fcWebMAudioEncoder::Vorbis: audio_encoder_name = "Vorbis"; break;
    case fcWebMAudioEncoder::Opus: audio_encoder_name = "Opus"; break;
    }

    char filename_f[256];
    char filename_c[256];
    char filename_m[256];
    sprintf(filename_f, "file_stream (%s %s).webm", video_encoder_name, audio_encoder_name);
    sprintf(filename_c, "custom_stream (%s %s).webm", video_encoder_name, audio_encoder_name);
    sprintf(filename_m, "memory_stream (%s %s).webm", video_encoder_name, audio_encoder_name);


    // create output streams
    fcStream* fstream = fcCreateFileStream(filename_f);
    fcStream* mstream = fcCreateMemoryStream();
    FILE *ofile = fopen(filename_c, "wb");
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
                CreateVideoData(video_frame.data(), Width, Height, i);
                fcWebMAddVideoFramePixels(ctx, video_frame.data(), fcPixelFormat_RGBAu8, t);
                t += 1.0 / FrameRate;
            }
        });

        // add audio frames
        std::thread audio_thread = std::thread([&]() {
            RawVector<float> audio_sample(SamplingRate);
            fcTime t = 0;
            while (t < (double)DurationInSeconds) {
                CreateAudioData(audio_sample.data(), (int)audio_sample.size(), t, 1.0f);
                fcWebMAddAudioFrame(ctx, audio_sample.data(), (int)audio_sample.size(), t);
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
        std::fstream of(filename_m, std::ios::binary | std::ios::out);
        of.write((char*)bd.data, bd.size);
    }
    fcDestroyStream(fstream);
    fcDestroyStream(mstream);
    fcDestroyStream(cstream);
    fclose(ofile);
}


void WebMTest()
{
    printf("WebMTest (VP8 & Vorbis) begin\n");
    WebMTest(fcWebMVideoEncoder::VP8, fcWebMAudioEncoder::Vorbis);
    printf("WebMTest (VP8 & Vorbis) end\n");

    printf("WebMTest (VP9 & Opus) begin\n");
    WebMTest(fcWebMVideoEncoder::VP9, fcWebMAudioEncoder::Opus);
    printf("WebMTest (VP9 & Opus) end\n");
}

