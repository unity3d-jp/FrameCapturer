#include "pch.h"
#include "TestCommon.h"

void WaveTest(int bits)
{
    const int SamplingRate = 48000;
    const int DurationInSeconds = 10;

    char filename[32];
    sprintf(filename, "test%dbits.wav", bits);

    fcWaveConfig conf;
    conf.sample_rate = SamplingRate;
    conf.num_channels = 1;
    conf.bits_per_sample = bits;
    fcStream *fstream = fcCreateFileStream(filename);
    fcIWaveContext *ctx = fcWaveCreateContext(&conf);
    fcWaveAddOutputStream(ctx, fstream);


    // add audio frames
    {
        RawVector<float> audio_sample(SamplingRate);
        fcTime t = 0;
        while (t < (double)DurationInSeconds) {
            CreateAudioData(audio_sample.data(), (int)audio_sample.size(), t, 1.0f);
            fcWaveAddAudioFrame(ctx, audio_sample.data(), (int)audio_sample.size());
            t += 1.0;
        }
    }

    fcReleaseContext(ctx);
    fcReleaseStream(fstream);
}

void WaveTest()
{
    if (!fcWaveIsSupported()) {
        printf("WaveTest: wave is not supported\n");
        return;
    }
    WaveTest(8);
    WaveTest(16);
    WaveTest(24);
}
