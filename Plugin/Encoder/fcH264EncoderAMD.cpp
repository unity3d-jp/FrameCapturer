#include "pch.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcSupportH264_AMD

#include "amf/public/common/AMFFactory.h"
#include "amf/public/include/components/VideoEncoderVCE.h"


class fcH264EncoderAMD : public fcIH264Encoder
{
public:
    fcH264EncoderAMD(const fcH264EncoderConfig& conf, void *device, fcHWEncoderDeviceType type);
    ~fcH264EncoderAMD() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe) override;

    bool isValid() { return m_encoder != nullptr; }

private:
    fcH264EncoderConfig m_conf;
    amf::AMFContextPtr m_ctx;
    amf::AMFComponentPtr m_encoder;

    Buffer m_rgba_image;
    I420Image m_i420_image;
};



static AMFFactoryHelper g_amf_helper;
static amf::AMFFactory *iamf = nullptr;

static bool LoadAMFModule()
{
    if (iamf) { return true; }

    if (g_amf_helper.Init() == AMF_OK) {
        iamf = g_amf_helper.GetFactory();
        return true;
    }
    return false;
}




fcH264EncoderAMD::fcH264EncoderAMD(const fcH264EncoderConfig& conf, void *device, fcHWEncoderDeviceType type)
    : m_conf(conf)
{
    if (!LoadAMFModule()) { return; }

    amf::AMFContextPtr ctx;
    amf::AMFComponentPtr encoder;
    if (iamf->CreateContext(&ctx) != AMF_OK) {
        return;
    }

    {
        bool ok = false;
        switch (type) {
        case fcHWEncoderDeviceType::D3D9:
            ok = ctx->InitDX9(device) == AMF_OK;
            break;
        case fcHWEncoderDeviceType::D3D11:
            ok = ctx->InitDX11(device) == AMF_OK;
            break;
        }
        if (!ok) { return; }
    }

    if (iamf->CreateComponent(ctx, AMFVideoEncoderVCE_AVC, &encoder) != AMF_OK) {
        return;
    }
    if (encoder->Init(amf::AMF_SURFACE_YUV420P, m_conf.width, m_conf.height) != AMF_OK) {
        return;
    }

    m_ctx = ctx;
    m_encoder = encoder;
}

fcH264EncoderAMD::~fcH264EncoderAMD()
{
}

const char* fcH264EncoderAMD::getEncoderInfo() { return "AMD H264 Encoder"; }


bool fcH264EncoderAMD::encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe)
{
    if (!isValid()) { return false; }

    AnyToI420(m_i420_image, m_rgba_image, image, fmt, m_conf.width, m_conf.height);
    I420Data i420 = m_i420_image.data();

    //// todo
    //amf::AMFDataPtr input;
    //m_encoder->SubmitInput(input);
    //amf::AMFDataPtr output;
    //m_encoder->QueryOutput(&output);

    return true;
}

fcIH264Encoder* fcCreateH264EncoderAMD(const fcH264EncoderConfig& conf, void *device, fcHWEncoderDeviceType type)
{
    auto *ret = new fcH264EncoderAMD(conf, device, type);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

#else  // fcSupportH264_AMD

fcIH264Encoder* fcCreateH264EncoderAMD(const fcH264EncoderConfig& conf, void *device, fcHWEncoderDeviceType type)
{
    return nullptr;
 }

#endif // fcSupportH264_AMD
