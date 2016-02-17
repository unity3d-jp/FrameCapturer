#ifndef fcAACEncoder_h
#define fcAACEncoder_h


struct fcAACEncoderConfig
{
    int sampling_rate;
    int num_channels;
    int target_bitrate;

    fcAACEncoderConfig() : sampling_rate(), num_channels(), target_bitrate() {}
};

class fcIAACEncoder
{
public:
    virtual ~fcIAACEncoder() {}
    virtual const char* getEncoderName() = 0;
    virtual const Buffer& getEncoderInfo() = 0;
    virtual bool encode(fcAACFrame& dst, const float *samples, int num_samples) = 0;
};

bool fcLoadFAACModule();
fcIAACEncoder* fcCreateFAACEncoder(const fcAACEncoderConfig& conf);


#endif // fcAACEncoder_h
