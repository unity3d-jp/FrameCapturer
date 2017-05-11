#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

#ifdef fcSupportH264_Intel

#include "IntelQSV/mfxvideo++.h"
#ifdef _WIN32
    #pragma comment(lib, "libmfx.lib")
    #pragma comment(lib, "legacy_stdio_definitions.lib")
#endif


class fcH264EncoderIntel : public fcIH264Encoder
{
public:
    fcH264EncoderIntel(const fcH264EncoderConfig& conf, int impl,
        mfxVersion *ver = nullptr, void *device = nullptr, fcHWEncoderDeviceType type = fcHWEncoderDeviceType::Unknown);
    ~fcH264EncoderIntel() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe) override;
    bool flush(fcH264Frame& dst) override;

    bool isValid() const { return m_encoder != nullptr; }

private:
    class TaskUnit
    {
    public:
        Buffer image_rgba;
        NV12Image image_nv12;
        mfxFrameSurface1 surface;

        Buffer out_data;
        mfxBitstream bitstream;
        mfxSyncPoint syncp = nullptr;

        TaskUnit(int w, int h, const mfxFrameInfo &fi);
    };
    static const int MaxTasks = 16;

    fcH264EncoderConfig m_conf;
    const char *m_encoder_name = nullptr;
    std::unique_ptr<MFXVideoSession> m_session;
    std::unique_ptr<MFXVideoENCODE> m_encoder;
    mfxVideoParam m_params;
    std::unique_ptr<TaskUnit> m_task_units[MaxTasks];
    int m_frame = 0;
};



fcH264EncoderIntel::TaskUnit::TaskUnit(int w, int h, const mfxFrameInfo &fi)
{
    image_rgba.resize(w * h * 4);
    image_nv12.resize(w, h);
    auto& data = image_nv12.data();

    memset(&surface, 0, sizeof(surface));
    surface.Info = fi;
    surface.Data.MemType = MFX_MEMTYPE_SYSTEM_MEMORY;
    surface.Data.Y = (mfxU8*)data.y;
    surface.Data.UV = (mfxU8*)data.uv;
    surface.Data.V = (mfxU8*)data.uv + 1;
    surface.Data.Pitch = data.pitch_y;
    surface.Data.FrameOrder = MFX_FRAMEORDER_UNKNOWN;

    out_data.resize(w * h * 2);
    memset(&bitstream, 0, sizeof(bitstream));
    bitstream.Data = (mfxU8*)out_data.data();
    bitstream.MaxLength = (mfxU32)out_data.size();
}


fcH264EncoderIntel::fcH264EncoderIntel(const fcH264EncoderConfig& conf, int impl, mfxVersion *ver, void *device, fcHWEncoderDeviceType type)
    : m_conf(conf)
{
    //switch (type) {
    //case fcHWEncoderDeviceType::D3D11:
    //    impl |= MFX_IMPL_VIA_D3D11;
    //    break;
    //}
    mfxStatus ret;
    m_session.reset(new MFXVideoSession());
    ret = m_session->Init(impl, ver);
    if (ret < 0) {
        m_session.reset();
        return;
    }

    //switch (type) {
    //case fcHWEncoderDeviceType::D3D11:
    //    m_session->SetHandle(MFX_HANDLE_D3D11_DEVICE, device);
    //    break;
    //}

    if (impl == MFX_IMPL_SOFTWARE) {
        m_encoder_name = "Intel H264 Encoder (SW)";
    }
    else {
        m_encoder_name = "Intel H264 Encoder (HW)";
    }

    auto& params = m_params;
    memset(&params, 0, sizeof(params));

    auto& fi = params.mfx.FrameInfo;
    fi.FourCC = MFX_FOURCC_NV12;
    fi.Width = roundup<16>(conf.width);
    fi.Height = roundup<16>(conf.height);
    fi.CropX = 0;
    fi.CropY = 0;
    fi.CropW = conf.width;
    fi.CropH = conf.height;
    fi.FrameRateExtN = conf.target_framerate;
    fi.FrameRateExtD = 1;
    fi.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    fi.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    params.mfx.CodecId = MFX_CODEC_AVC;
    params.mfx.CodecProfile = MFX_PROFILE_AVC_HIGH;
    params.mfx.GopOptFlag = MFX_GOP_CLOSED;
    params.mfx.GopPicSize = 0;
    params.mfx.GopRefDist = 0;
    params.mfx.TargetUsage = MFX_TARGETUSAGE_BEST_SPEED;

    switch (m_conf.bitrate_mode) {
    case fcBitrateMode::CBR:
        params.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        break;
    case fcBitrateMode.VBR:
        params.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        break;
    }
    params.mfx.TargetKbps = conf.target_bitrate / 1000;
    params.mfx.MaxKbps = 0;
    params.mfx.BufferSizeInKB = (fi.Width * fi.Height * 2) / 1024;
    params.mfx.NumSlice = 0;
    params.mfx.NumRefFrame = 0;
    params.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

    m_encoder.reset(new MFXVideoENCODE(*m_session));
    ret = m_encoder->Init(&params);
    if (ret < 0) {
        m_encoder.reset();
    }

    for (auto& tu : m_task_units) {
        tu.reset(new TaskUnit(m_conf.width, m_conf.height, params.mfx.FrameInfo));
    }
}

fcH264EncoderIntel::~fcH264EncoderIntel()
{
    m_encoder.reset();
    m_session.reset();
}

const char* fcH264EncoderIntel::getEncoderInfo() { return m_encoder_name; }

#define MSDK_DEC_WAIT_INTERVAL 300000
#define MSDK_ENC_WAIT_INTERVAL 300000
#define MSDK_VPP_WAIT_INTERVAL 300000
#define MSDK_SURFACE_WAIT_INTERVAL 20000
#define MSDK_DEVICE_FREE_WAIT_INTERVAL 30000
#define MSDK_WAIT_INTERVAL MSDK_DEC_WAIT_INTERVAL+3*MSDK_VPP_WAIT_INTERVAL+MSDK_ENC_WAIT_INTERVAL // an estimate for the longest pipeline we have in samples

bool fcH264EncoderIntel::encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe)
{
    if (!isValid()) { return false; }

    TaskUnit& tu = *m_task_units[m_frame++ % MaxTasks];

    // convert image to NV12
    AnyToNV12(tu.image_nv12, tu.image_rgba, image, fmt, m_conf.width, m_conf.height);
    NV12Data data = tu.image_nv12.data();


    dst.timestamp = timestamp;

    auto& surface = tu.surface;
    auto& bitstream = tu.bitstream;
    auto& syncp = tu.syncp;
    surface.Data.TimeStamp = (mfxU64)(timestamp * 90000.0); // unit is 90KHz
    bitstream.DataLength = 0;
    bitstream.DataOffset = 0;
    bitstream.DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;
    bitstream.TimeStamp = surface.Data.TimeStamp;
    syncp = nullptr;

    // encode!
    mfxStatus ret;
    for (;;) {
        ret = m_encoder->EncodeFrameAsync(nullptr, &surface, &bitstream, &syncp);
        if (ret == MFX_WRN_DEVICE_BUSY) {
            MilliSleep(1);
        }
        else { break; }
    }

    if (syncp) {
        // wait encode complete
        ret = m_session->SyncOperation(syncp, MSDK_WAIT_INTERVAL);
        if (ret < 0) { return false; }

        {
            dst.data.append((char*)bitstream.Data, bitstream.DataLength);
            dst.gatherNALInformation();
        }

        {
            // convert frame type
            dst.type = 0;
            int t = bitstream.FrameType;
            if ((t & MFX_FRAMETYPE_I) != 0) dst.type |= fcH264FrameType_I;
            if ((t & MFX_FRAMETYPE_P) != 0) dst.type |= fcH264FrameType_P;
            if ((t & MFX_FRAMETYPE_B) != 0) dst.type |= fcH264FrameType_B;
            if ((t & MFX_FRAMETYPE_S) != 0) dst.type |= fcH264FrameType_S;
            if ((t & MFX_FRAMETYPE_REF) != 0) dst.type |= fcH264FrameType_REF;
            if ((t & MFX_FRAMETYPE_IDR) != 0) dst.type |= fcH264FrameType_IDR;
        }
    }

    return true;
}

bool fcH264EncoderIntel::flush(fcH264Frame& dst)
{
    return false;
}

fcIH264Encoder* fcCreateH264EncoderIntelHW(const fcH264EncoderConfig& conf, void *device, fcHWEncoderDeviceType type)
{
    mfxVersion ver = { 0, 1 };
    auto ret = new fcH264EncoderIntel(conf, MFX_IMPL_HARDWARE_ANY, &ver, device, type);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

fcIH264Encoder* fcCreateH264EncoderIntelSW(const fcH264EncoderConfig& conf)
{
    auto ret = new fcH264EncoderIntel(conf, MFX_IMPL_SOFTWARE);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

#else // fcSupportH264_Intel

fcIH264Encoder* fcCreateH264EncoderIntelHW(const fcH264EncoderConfig& conf, void *device, fcHWEncoderDeviceType type) { return nullptr; }
fcIH264Encoder* fcCreateH264EncoderIntelSW(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportH264_Intel
