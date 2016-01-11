#ifndef fcMP4Encoder_h
#define fcMP4Encoder_h

class ISVCEncoder;


class fcH264Encoder
{
public:
    static bool loadModule();

    fcH264Encoder(int width, int height, float frame_rate, int target_bitrate);
    ~fcH264Encoder();
    operator bool() const;
    fcH264Frame encodeI420(const void *src_y, const void *src_u, const void *src_v);

private:
    ISVCEncoder *m_encoder;
    int m_width;
    int m_height;
};

#endif // fcMP4Encoder_h
