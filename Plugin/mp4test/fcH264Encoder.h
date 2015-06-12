#ifndef fcMP4Encoder_h
#define fcMP4Encoder_h

#include <functional>
#include <string>
#include "fcColorSpace.h"

class ISVCEncoder;

class fcH264Encoder
{
public:
    enum FrameType
    {
        videoFrameTypeInvalid,    ///< encoder not ready or parameters are invalidate
        videoFrameTypeIDR,        ///< IDR frame in H.264
        videoFrameTypeI,          ///< I frame type
        videoFrameTypeP,          ///< P frame type
        videoFrameTypeSkip,       ///< skip the frame based encoder kernel
        videoFrameTypeIPMixed     ///< a frame where I and P slices are mixing, not supported yet
    };
    struct Result
    {
        void *data;
        int size;
        FrameType type;

        Result(void *d = nullptr, int s = 0, FrameType t = videoFrameTypeInvalid)
            : data(d), size(s), type(t) {}
    };

    fcH264Encoder(int width, int height, float frame_rate, int target_bitrate);
    ~fcH264Encoder();
    operator bool() const;
    Result encodeRGBA(const bRGBA *src);
    Result encodeI420(const void *src_y, const void *src_u, const void *src_v);

private:
    ISVCEncoder *m_encoder;
    std::string m_buf;
    int m_width;
    int m_height;
};


#endif // fcMP4Encoder_h
