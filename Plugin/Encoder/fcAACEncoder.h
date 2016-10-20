#ifndef fcAACEncoder_h
#define fcAACEncoder_h


struct fcAACEncoderConfig
{
    int sample_rate;
    int num_channels;
    int target_bitrate;

    fcAACEncoderConfig() : sample_rate(), num_channels(), target_bitrate() {}
};

class fcIAACEncoder
{
public:
    virtual ~fcIAACEncoder() {}
    virtual const char* getEncoderInfo() = 0;
    virtual const Buffer& getDecoderSpecificInfo() = 0;
    virtual bool encode(fcAACFrame& dst, const float *samples, size_t num_samples) = 0;
};

bool fcDownloadFAAC(fcDownloadCallback cb);
bool fcLoadFAACModule();
fcIAACEncoder* fcCreateFAACEncoder(const fcAACEncoderConfig& conf);


#endif // fcAACEncoder_h
