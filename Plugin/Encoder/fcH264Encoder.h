#ifndef fcMP4Encoder_h
#define fcMP4Encoder_h

#include <string>

class ISVCEncoder;

class fcH264Encoder
{
public:
    enum FrameType
    {
        FrameType_Invalid,    ///< encoder not ready or parameters are invalidate
        FrameType_IDR,        ///< IDR frame in H.264
        FrameType_I,          ///< I frame type
        FrameType_P,          ///< P frame type
        FrameType_Skip,       ///< skip the frame based encoder kernel
        FrameType_IPMixed     ///< a frame where I and P slices are mixing, not supported yet
    };
    struct Result
    {
        void *data;
        int size;
        FrameType type;

        Result(void *d = nullptr, int s = 0, FrameType t = FrameType_Invalid)
            : data(d), size(s), type(t) {}
    };

    static bool loadModule();

    fcH264Encoder(int width, int height, float frame_rate, int target_bitrate);
    ~fcH264Encoder();
    operator bool() const;
    Result encodeRGBA(const uint8_t *rgba);
    Result encodeI420(const void *src_y, const void *src_u, const void *src_v);

private:
    ISVCEncoder *m_encoder;
    std::string m_buf;
    int m_width;
    int m_height;
};


#endif // fcMP4Encoder_h
