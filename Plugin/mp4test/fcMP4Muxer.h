#ifndef fcMP4Muxer_h
#define fcMP4Muxer_h

#include <iostream>

class fcMP4Muxer
{
public:
    fcMP4Muxer();
    ~fcMP4Muxer();

    // equvalant to: ffmpeg -f h264 -i in_h264_path -c:v copy out_mp4_path
    bool mux(const char *out_mp4_path, const char *in_h264_path, int frame_rate);
    //bool mux(std::ostream *out, std::istream *in_h264, int frame_rate); // todo
};

#endif // fcMP4Muxer_h
