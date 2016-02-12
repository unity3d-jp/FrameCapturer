#include "pch.h"
#include <libyuv/libyuv.h>
#include "fcFoundation.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

class fcNVH264Encoder : public fcIH264Encoder
{
public:
    fcNVH264Encoder(const fcH264EncoderConfig& conf);
    ~fcNVH264Encoder();
    bool encode(fcH264Frame& dst, const fcI420Image& image, uint64_t timestamp, bool force_keyframe) override;

private:
    fcH264EncoderConfig m_conf;
};

fcNVH264Encoder::fcNVH264Encoder(const fcH264EncoderConfig& conf)
    : m_conf(conf)
{
}

fcNVH264Encoder::~fcNVH264Encoder()
{
}

bool fcNVH264Encoder::encode(fcH264Frame& dst, const fcI420Image& image, uint64_t timestamp, bool force_keyframe)
{
    return false;
}

fcIH264Encoder* fcCreateNVH264Encoder(const fcH264EncoderConfig& conf)
{
    return new fcNVH264Encoder(conf);
}
