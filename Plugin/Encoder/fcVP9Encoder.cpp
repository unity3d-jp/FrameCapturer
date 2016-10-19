#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcVPXEncoder.h"

#include "vpx/video_common.h"

class fcVP9Encoder : public fcIVPXEncoder
{
public:
    fcVP9Encoder(const fcVP9EncoderConfig& conf);
    ~fcVP9Encoder() override;
    void release() override;
    bool encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe) override;

private:
    fcVP9EncoderConfig m_conf;
};


fcIVPXEncoder* fcCreateVP9Encoder(const fcVP9EncoderConfig& conf)
{
    return new fcVP9Encoder(conf);
}

fcVP9Encoder::fcVP9Encoder(const fcVP9EncoderConfig& conf)
    : m_conf(conf)
{
}

fcVP9Encoder::~fcVP9Encoder()
{
}

void fcVP9Encoder::release()
{
    delete this;
}

bool fcVP9Encoder::encode(fcVPXFrame& dst, const fcI420Data& image, fcTime timestamp, bool force_keyframe)
{
    return false;
}
