#include "pch.h"
#include "TestCommon.h"

void PngTest();
void ExrTest();
void GifTest();
void MP4Test();
void WebMTest();
void WaveTest();
void OggTest();
void FlacTest();
void ConvertTest();

int main(int argc, char *argv[])
{
    bool png = false;
    bool exr = false;
    bool gif = false;
    bool mp4 = false;
    bool webm = false;
    bool wave = false;
    bool ogg = false;
    bool flac = false;
    bool convert = false;

    if (argc <= 1) {
        png = exr = gif = mp4 = webm = convert = true;
        //faac = true;
    }
    else {
        for (int i = 1; i < argc; ++i) {
            if      (strstr(argv[i], "png")) { png = true; }
            else if (strstr(argv[i], "exr")) { exr = true; }
            else if (strstr(argv[i], "gif")) { gif = true; }
            else if (strstr(argv[i], "mp4")) { mp4 = true; }
            else if (strstr(argv[i], "webm")) { webm = true; }
            else if (strstr(argv[i], "wave")) { wave = true; }
            else if (strstr(argv[i], "ogg")) { ogg = true; }
            else if (strstr(argv[i], "flac")) { flac = true; }
            else if (strstr(argv[i], "convert")) { convert = true; }
        }
    }

    InitializeD3D11();
    if (png) PngTest();
    if (exr) ExrTest();
    if (gif) GifTest();
    if (mp4) MP4Test();
    if (webm) WebMTest();
    if (wave) WaveTest();
    if (ogg) OggTest();
    if (flac) FlacTest();
    if (convert) ConvertTest();

    fcWaitAsyncDelete();
}
