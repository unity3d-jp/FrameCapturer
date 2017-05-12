#include "pch.h"
#include "TestCommon.h"

void OggTest()
{
    if (!fcOggIsSupported()) {
        printf("OggTest: ogg is not supported\n");
        return;
    }

    const int SamplingRate = 48000;
    const int DurationInSeconds = 10;

    char filename[32];
    sprintf(filename, "test.ogg");

    fcOggConfig conf;
    conf.sample_rate = SamplingRate;
    conf.num_channels = 1;
    fcStream *fstream = fcCreateFileStream(filename);
    fcIOggContext *ctx = fcOggCreateContext(&conf);
    fcOggAddOutputStream(ctx, fstream);


    // add audio frames
    {
        RawVector<float> audio_sample(SamplingRate);
        fcTime t = 0;
        while (t < (double)DurationInSeconds) {
            CreateAudioData(audio_sample.data(), (int)audio_sample.size(), t, 1.0f);
            fcOggAddAudioFrame(ctx, audio_sample.data(), (int)audio_sample.size());
            t += 1.0;
        }
    }

    fcReleaseContext(ctx);
    fcReleaseStream(fstream);
}
