#include "pch.h"
#include "TestCommon.h"


// custom stream functions (just a wrapper of FILE)
static size_t tellp(void *f) { return ftell((FILE*)f); }
static void   seekp(void *f, size_t pos) { fseek((FILE*)f, (long)pos, SEEK_SET); }
static size_t write(void *f, const void *data, size_t len) { return fwrite(data, 1, len, (FILE*)f); }

struct WebMTestContext
{
    char filename_f[256];
    char filename_c[256];
    char filename_m[256];
    fcStream* fstream = nullptr;
    fcStream* mstream = nullptr;
    fcStream* cstream = nullptr;
    FILE *ofile;

    WebMTestContext(const fcWebMConfig& conf);
    ~WebMTestContext();
    static void Release(void *_this);
};

WebMTestContext::WebMTestContext(const fcWebMConfig& conf)
{
    const char *video_encoder_name = nullptr;
    const char *audio_encoder_name = nullptr;
    switch (conf.video_encoder) {
    case fcWebMVideoEncoder::VPX_VP8: video_encoder_name = "VP8"; break;
    case fcWebMVideoEncoder::VPX_VP9: video_encoder_name = "VP9"; break;
    }
    switch (conf.audio_encoder) {
    case fcWebMAudioEncoder::Vorbis: audio_encoder_name = "Vorbis"; break;
    case fcWebMAudioEncoder::Opus: audio_encoder_name = "Opus"; break;
    }
    sprintf(filename_f, "file_stream (%s %s).webm", video_encoder_name, audio_encoder_name);
    sprintf(filename_c, "custom_stream (%s %s).webm", video_encoder_name, audio_encoder_name);
    sprintf(filename_m, "memory_stream (%s %s).webm", video_encoder_name, audio_encoder_name);

    fstream = fcCreateFileStream(filename_f);
    mstream = fcCreateMemoryStream();
    ofile = fopen(filename_c, "wb");
    cstream = fcCreateCustomStream(ofile, &tellp, &seekp, &write);
}

WebMTestContext::~WebMTestContext()
{
    {
        fcBufferData bd = fcStreamGetBufferData(mstream);
        std::fstream of(filename_m, std::ios::binary | std::ios::out);
        of.write((char*)bd.data, bd.size);
    }
    fcReleaseStream(fstream);
    fcReleaseStream(mstream);
    fcReleaseStream(cstream);
    fclose(ofile);
}

void WebMTestContext::Release(void *_this)
{
    delete (WebMTestContext*)_this;
}


void WebMTest(fcWebMVideoEncoder ve, fcWebMAudioEncoder ae)
{
    const int DurationInSeconds = 10;
    const int FrameRate = 60;
    const int Width = 320;
    const int Height = 240;
    const int SamplingRate = 48000;

    fcWebMConfig conf;
    conf.video = true;
    conf.audio = true;
    conf.video_encoder = ve;
    conf.video_width = Width;
    conf.video_height = Height;
    //conf.video_bitrate_mode = fcBitrateMode.VBR;
    conf.video_target_bitrate = 256 * 1000;
    conf.audio_encoder = ae;
    conf.audio_sample_rate = SamplingRate;
    conf.audio_num_channels = 1;
    conf.audio_target_bitrate = 64 * 1000;

    // create mp4 context and add output streams
    fcIWebMContext *ctx = fcWebMCreateContext(&conf);

    // create streams
    auto *testctx = new WebMTestContext(conf);
    fcWebMAddOutputStream(ctx, testctx->fstream);
    fcWebMAddOutputStream(ctx, testctx->mstream);
    fcWebMAddOutputStream(ctx, testctx->cstream);
    fcSetOnDeleteCallback(ctx, &WebMTestContext::Release, testctx);

    // create movie data
    {
        // add video frames
        std::thread video_thread = std::thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
                fcWebMAddAudioSamples(ctx, audio_sample.data(), (int)audio_sample.size());
                t += 1.0;
            }
        });

        // wait
        video_thread.join();
        audio_thread.join();
    }

    // destroy mp4 context
    fcReleaseContext(ctx);
}


void WebMTest()
{
    if (!fcWebMIsSupported()) {
        printf("WebMTest: webm is not supported\n");
        return;
    }

    printf("WebMTest (VP8 & Vorbis) begin\n");
    WebMTest(fcWebMVideoEncoder::VPX_VP8, fcWebMAudioEncoder::Vorbis);
    printf("WebMTest (VP8 & Vorbis) end\n");

    printf("WebMTest (VP9 & Opus) begin\n");
    WebMTest(fcWebMVideoEncoder::VPX_VP9, fcWebMAudioEncoder::Opus);
    printf("WebMTest (VP9 & Opus) end\n");
}

