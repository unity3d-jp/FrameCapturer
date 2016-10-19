#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcVPXEncoder.h"

#include "vpx/vpx/vpx_encoder.h"
#include "vpx/vpx/vp8cx.h"

class fcVP8Encoder : public fcIVPXEncoder
{
public:
    fcVP8Encoder(const fcVP8EncoderConfig& conf);
    ~fcVP8Encoder() override;
    void release() override;
    bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe) override;

private:
    fcVP8EncoderConfig m_conf;
};


fcIVPXEncoder* fcCreateVP8Encoder(const fcVP8EncoderConfig& conf)
{
    return new fcVP8Encoder(conf);
}

fcVP8Encoder::fcVP8Encoder(const fcVP8EncoderConfig& conf)
    : m_conf(conf)
{
}

fcVP8Encoder::~fcVP8Encoder()
{
}

void fcVP8Encoder::release()
{
    delete this;
}

bool fcVP8Encoder::encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe)
{
    return false;
}
