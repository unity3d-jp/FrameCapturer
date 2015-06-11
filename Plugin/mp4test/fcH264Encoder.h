#ifndef fcMP4Encoder_h
#define fcMP4Encoder_h

#include <functional>
#include <string>
#include "fcColorSpace.h"

class ISVCEncoder;

class fcH264Encoder
{
public:
    struct result
    {
        void *data;
        int data_size;
    };

    fcH264Encoder(int width, int height, float frame_rate, int target_bitrate);
    ~fcH264Encoder();
    operator bool() const;
    result encodeRGBA(const bRGBA *src);
    result encodeI420(const void *src_y, const void *src_u, const void *src_v);

private:
    ISVCEncoder *m_encoder;
    std::string m_buf;
    int m_width;
    int m_height;
};


#endif // fcMP4Encoder_h
