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
    bool flush(fcH264Frame& dst) override;

    bool isValid() const { return m_encoder != nullptr; }

private:
    fcH264EncoderConfig m_conf;
    amf::AMFContextPtr m_ctx;
    amf::AMFComponentPtr m_encoder;
    amf::AMFSurfacePtr m_surface;

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
    amf::AMFSurfacePtr surface;

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

    ctx->AllocSurface(amf::AMF_MEMORY_HOST, amf::AMF_SURFACE_YUV420P, m_conf.width, m_conf.height, &m_surface);

    m_ctx = ctx;
    m_encoder = encoder;
    m_surface = surface;
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

    memcpy(m_surface->GetPlane(amf::AMF_PLANE_Y)->GetNative(), i420.y, i420.pitch_y * i420.height);
    memcpy(m_surface->GetPlane(amf::AMF_PLANE_U)->GetNative(), i420.u, i420.pitch_u * i420.height);
    memcpy(m_surface->GetPlane(amf::AMF_PLANE_V)->GetNative(), i420.v, i420.pitch_v * i420.height);

    m_encoder->SubmitInput(m_surface);

    amf::AMFDataPtr out_data;
    m_encoder->QueryOutput(&out_data);
    out_data->Convert(amf::AMF_MEMORY_HOST);

    amf::AMFBufferPtr out_buf(out_data);
    dst.data.append((char*)out_buf->GetNative(), out_buf->GetSize());
    dst.gatherNALInformation();

    return true;
}

bool fcH264EncoderAMD::flush(fcH264Frame& dst)
{
    return false;
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
