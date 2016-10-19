#include "TestCommon.h"

void PngTest();
void ExrTest();
void GifTest();
void MP4Test();
void WebMTest();
void ConvertTest();
void FAACSelfBuildTest();

int main(int argc, char *argv[])
{
    bool png = false;
    bool exr = false;
    bool gif = false;
    bool mp4 = false;
    bool webm = false;
    bool convert = false;
    bool faac = false;

    if (argc <= 1) {
        png = exr = gif = mp4 = webm = convert = true;
        //faac = true;
    }
    else {
        for (int i = 1; i < argc; ++i) {
            if      (strstr(argv[i], "png")) { png = true; }
            else if (strstr(argv[i], "exr")) { exr = true; }
            else if (strstr(argv[i], "gif")) { gif = true; }
            else if (strstr(argv[i], "faac")) { faac = true; }
            else if (strstr(argv[i], "mp4")) { mp4 = true; }
            else if (strstr(argv[i], "webm")) { webm = true; }
            else if (strstr(argv[i], "convert")) { convert = true; }
        }
    }

    if (png) PngTest();
    if (exr) ExrTest();
    if (gif) GifTest();
    if (mp4) MP4Test();
    if (webm) WebMTest();
    if (convert) ConvertTest();
    if (faac) FAACSelfBuildTest();
}
