#include "TestCommon.h"
using namespace std::literals;


// custom stream functions (just a wrapper of FILE)
static size_t tellp(void *f) { return ftell((FILE*)f); }
static void   seekp(void *f, size_t pos) { fseek((FILE*)f, (long)pos, SEEK_SET); }
static size_t write(void *f, const void *data, size_t len) { return fwrite(data, 1, len, (FILE*)f); }


void MP4Test()
{
    printf("MP4Test begin\n");


    // download OpenH264 codec
    fcMP4DownloadCodecBegin();
    for (int i = 0; i < 30; ++i) {
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

    // create movie data
    {
        // add video frames
        std::thread video_thread = std::thread([&]() {
            RawVector<RGBAu8> video_frame(Width * Height);
            fcTime t = 0;
            for (int i = 0; i < DurationInSeconds * FrameRate; ++i) {
                CreateVideoData(&video_frame[0], Width, Height, i);
                fcMP4AddVideoFramePixels(ctx, &video_frame[0], fcPixelFormat_RGBAu8, t);
                t += 1.0 / FrameRate;
            }
        });

        // add audio frames
        std::thread audio_thread = std::thread([&]() {
            RawVector<float> audio_sample(SamplingRate);
            fcTime t = 0;
            for (int i = 0; i < DurationInSeconds; ++i) {
                CreateAudioData(&audio_sample[0], (int)audio_sample.size(), i, 32767.0f);
                fcMP4AddAudioFrame(ctx, &audio_sample[0], (int)audio_sample.size(), t);
                t += 1.0;
            }
        });

        // wait
        video_thread.join();
        audio_thread.join();
    }

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

    printf("MP4Test end\n");
}

