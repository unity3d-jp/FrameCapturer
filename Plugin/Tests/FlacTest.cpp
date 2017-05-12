#include "pch.h"
#include "TestCommon.h"


void FlacTest(int bits)
{
    if (!fcFlacIsSupported()) {
        printf("FlacTest: flac is not supported\n");
        return;
    }

    const int SamplingRate = 48000;
    const int DurationInSeconds = 10;

    char filename[32];
    sprintf(filename, "test%dbits.flac", bits);

    fcFlacConfig conf;
    conf.sample_rate = SamplingRate;
    conf.num_channels = 1;
    conf.bits_per_sample = bits;
    fcStream *fstream = fcCreateFileStream(filename);
    fcIFlacContext *ctx = fcFlacCreateContext(&conf);
    fcFlacAddOutputStream(ctx, fstream);


    // add audio frames
    {
        RawVector<float> audio_sample(SamplingRate);
        fcTime t = 0;
        while (t < (double)DurationInSeconds) {
            CreateAudioData(audio_sample.data(), (int)audio_sample.size(), t, 1.0f);
            fcFlacAddAudioFrame(ctx, audio_sample.data(), (int)audio_sample.size());
            t += 1.0;
        }
    }

    fcFlacDestroyContext(ctx);
    fcDestroyStream(fstream);
}

void FlacTest()
{
    FlacTest(8);
    FlacTest(16);
    FlacTest(24);
}
