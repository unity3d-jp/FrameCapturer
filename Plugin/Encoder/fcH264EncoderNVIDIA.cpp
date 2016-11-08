#include "pch.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"
#include "NVENC/nvEncodeAPI.h"

#ifdef fcSupportH264_NVIDIA

#ifdef _WIN32
    #if defined(_M_AMD64)
        #define NVEncoderDLL    "nvEncodeAPI64.dll"
    #elif defined(_M_IX86)
        #define NVEncoderDLL    "nvEncodeAPI.dll"
    #endif
#else
    #define NVEncoderDLL    "libnvidia-encode.so.1"
#endif


class fcH264EncoderNVIDIA : public fcIH264Encoder
{
public:
    fcH264EncoderNVIDIA(const fcH264EncoderConfig& conf, void *device, fcNVENCDeviceType type);
    ~fcH264EncoderNVIDIA() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe) override;

    bool isValid() { return m_encoder != nullptr; }

private:
    fcH264EncoderConfig m_conf;
    void *m_encoder = nullptr;
    Buffer m_rgba_image;
    NV12Image m_nv12_image;
};



static module_t g_mod_nvenc;
static NVENCSTATUS (NVENCAPI *NvEncodeAPICreateInstance_)(NV_ENCODE_API_FUNCTION_LIST *functionList);
static NV_ENCODE_API_FUNCTION_LIST nvenc;

static bool LoadNVENCModule()
{
    if (NvEncodeAPICreateInstance_) { return true; }

    g_mod_nvenc = DLLLoad(NVEncoderDLL);
    if (g_mod_nvenc) {
        (void*&)NvEncodeAPICreateInstance_ = DLLGetSymbol(g_mod_nvenc, "NvEncodeAPICreateInstance");
        if (NvEncodeAPICreateInstance_) {
            nvenc.version = NV_ENCODE_API_FUNCTION_LIST_VER;
            NvEncodeAPICreateInstance_(&nvenc);
        }
    }
    return NvEncodeAPICreateInstance_ != nullptr;
}


fcH264EncoderNVIDIA::fcH264EncoderNVIDIA(const fcH264EncoderConfig& conf, void *device, fcNVENCDeviceType type)
    : m_conf(conf)
{
    if (!LoadNVENCModule()) { return; }

    {
        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params;
        memset(&params, 0, sizeof(params));
        params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
        params.apiVersion = NVENCAPI_VERSION;
        params.device = device;
        switch (type) {
        case fcNVENCDeviceType_DirectX:
            params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
            break;
        case fcNVENCDeviceType_CUDA:
            params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
            break;
        }
        nvenc.nvEncOpenEncodeSessionEx(&params, &m_encoder);
        if (!m_encoder) {
            return;
        }
    }

    {
        NV_ENC_INITIALIZE_PARAMS params;
        memset(&params, 0, sizeof(params));
        params.version = NV_ENC_INITIALIZE_PARAMS_VER;
        params.encodeGUID = NV_ENC_CODEC_H264_GUID;
        params.presetGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
        params.encodeHeight = m_conf.width;
        params.encodeHeight = m_conf.height;
        params.darWidth = 1;
        params.darHeight = 1;
        params.frameRateNum = m_conf.target_framerate;
        params.frameRateDen = 1;
        nvenc.nvEncInitializeEncoder(m_encoder, &params);
    }
}

fcH264EncoderNVIDIA::~fcH264EncoderNVIDIA()
{
    if (m_encoder) {
        nvenc.nvEncDestroyEncoder(m_encoder);
    }
}

const char* fcH264EncoderNVIDIA::getEncoderInfo() { return "NVIDIA H264 Encoder"; }

bool fcH264EncoderNVIDIA::encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe)
{
    dst.timestamp = timestamp;

    // convert image to NV12
    AnyToNV12(m_nv12_image, m_rgba_image, image, fmt, m_conf.width, m_conf.height);
    NV12Data data = m_nv12_image.data();

    NV_ENC_PIC_PARAMS params;
    memset(&params, 0, sizeof(params));
    params.version = NV_ENC_PIC_PARAMS_VER;
    //params.inputBuffer = surf->inputSurface;
    params.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12;
    params.inputWidth = m_conf.width;
    params.inputHeight = m_conf.height;
    //params.outputBitstream = outputSurfaces[i].outputSurface;
    params.completionEvent = 0;
    params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    params.encodePicFlags = 0;
    params.inputTimeStamp = to_usec(timestamp);
    params.inputDuration = 0;

    nvenc.nvEncEncodePicture(m_encoder, &params);
    return false;
}

fcIH264Encoder* fcCreateH264EncoderNVIDIA(const fcH264EncoderConfig& conf, void *device, fcNVENCDeviceType type)
{
    auto *ret = new fcH264EncoderNVIDIA(conf, device, type);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

#else // fcSupportH264_NVIDIA

fcIH264Encoder* fcCreateH264EncoderNVIDIA(const fcH264EncoderConfig& conf, void *device, fcNVENCDeviceType type) { return nullptr; }

#endif // fcSupportH264_NVIDIA
