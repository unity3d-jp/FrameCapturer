#ifndef fcMP4Encoder_h
#define fcMP4Encoder_h

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
    struct FrameData
    {
        DataRef buf;
        FrameType type;

        FrameData(void *d = nullptr, int s = 0, FrameType t = FrameType_Invalid)
            : buf(d, s), type(t) {}
    };

    static bool loadModule();

    fcH264Encoder(int width, int height, float frame_rate, int target_bitrate);
    ~fcH264Encoder();
    operator bool() const;
    FrameData encodeI420(const void *src_y, const void *src_u, const void *src_v);

private:
    ISVCEncoder *m_encoder;
    int m_width;
    int m_height;
};


typedef void(*fcDownloadCallback)(bool is_complete, const char *status);
typedef bool (*fcMP4DownloadCodecImplT)(fcDownloadCallback cb);

#endif // fcMP4Encoder_h
