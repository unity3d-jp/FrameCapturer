#include "pch.h"
#include "TestCommon.h"

template<class T>
void GifTestImpl(const char *filename)
{
    const int Width = 320;
    const int Height = 240;
    const int frame_count = 30;

    fcGifConfig conf;
    conf.width = Width;
    conf.height = Height;
    fcStream *fstream = fcCreateFileStream(filename);
    fcIGifContext *ctx = fcGifCreateContext(&conf);
    fcGifAddOutputStream(ctx, fstream);

    fcTime t = 0;
    RawVector<T> video_frame(Width * Height);
    for (int i = 0; i < frame_count; ++i) {
        CreateVideoData(&video_frame[0], Width, Height, i);
        fcGifAddFramePixels(ctx, &video_frame[0], GetPixelFormat<T>::value, t);
        t += 1.0 / 30.0;
    }

    fcReleaseContext(ctx);
    fcReleaseStream(fstream);
}

void GifTest()
{
    if (!fcGifIsSupported()) {
        printf("GifTest: gif is not supported\n");
        return;
    }

    printf("GifTest begin\n");

    std::vector<std::future<void>> tasks;
    tasks.push_back(std::async(std::launch::async, []() { GifTestImpl<RGBu8>("RGBu8.gif");     }));
    tasks.push_back(std::async(std::launch::async, []() { GifTestImpl<RGBf16>("RGBf16.gif");   }));
    tasks.push_back(std::async(std::launch::async, []() { GifTestImpl<RGBf32>("RGBf32.gif");   }));
    tasks.push_back(std::async(std::launch::async, []() { GifTestImpl<RGBAu8>("RGBAu8.gif");   }));
    tasks.push_back(std::async(std::launch::async, []() { GifTestImpl<RGBAf16>("RGBAf16.gif"); }));
    tasks.push_back(std::async(std::launch::async, []() { GifTestImpl<RGBAf32>("RGBAf32.gif"); }));

    for (auto& task : tasks) { task.get(); }

    printf("GifTest end\n");
}

