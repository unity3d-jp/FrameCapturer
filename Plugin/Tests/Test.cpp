#include "TestCommon.h"

void PngTest();
void ExrTest();
void GifTest();
void MP4Test();

int main(int argc, char *argv[])
{
    bool png = false;
    bool exr = false;
    bool gif = false;
    bool mp4 = false;

    if (argc <= 1) {
        png = exr = gif = mp4 = true;
    }
    else {
        for (int i = 1; i < argc; ++i) {
            if      (strstr(argv[i], "png")) { png = true; }
            else if (strstr(argv[i], "exr")) { exr = true; }
            else if (strstr(argv[i], "gif")) { gif = true; }
            else if (strstr(argv[i], "mp4")) { mp4 = true; }
        }
    }

    if (png) PngTest();
    if (exr) ExrTest();
    if (gif) GifTest();
    if (mp4) MP4Test();
}
