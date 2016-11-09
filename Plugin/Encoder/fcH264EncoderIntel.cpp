#include "pch.h"
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
    fcH264EncoderIntel(const fcH264EncoderConfig& conf, int impl);
    ~fcH264EncoderIntel() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe) override;

    bool isValid() { return m_encoder != nullptr; }

private:
    fcH264EncoderConfig m_conf;
    const char *m_encoder_name = nullptr;
    std::unique_ptr<MFXVideoSession> m_session;
    std::unique_ptr<MFXVideoENCODE> m_encoder;
    mfxVideoParam m_params;
    Buffer m_rgba_image;
    NV12Image m_nv12_image;
    Buffer m_encoded_data;
};



fcH264EncoderIntel::fcH264EncoderIntel(const fcH264EncoderConfig& conf, int impl)
    : m_conf(conf)
{
    mfxStatus ret;
    m_session.reset(new MFXVideoSession());
    ret = m_session->Init(impl, nullptr);
    if (ret < 0) {
        m_session.reset();
        return;
    }

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
    case fcCBR:
        params.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        break;
    case fcVBR:
        params.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        break;
    }
    params.mfx.TargetKbps = conf.target_bitrate / 1000;
    params.mfx.MaxKbps = 0;
    params.mfx.BufferSizeInKB = (fi.Width * fi.Height * 2) / 1024;
    params.mfx.NumSlice = 0;
    params.mfx.NumRefFrame = 0;

    params.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    m_encoder.reset(new MFXVideoENCODE(*m_session));
    ret = m_encoder->Init(&params);
    if (ret < 0) {
        m_encoder.reset();
    }

    m_encoded_data.resize(params.mfx.BufferSizeInKB * 1024);
}

fcH264EncoderIntel::~fcH264EncoderIntel()
{
    m_encoder.reset();
    m_session.reset();
}

const char* fcH264EncoderIntel::getEncoderInfo() { return m_encoder_name; }

bool fcH264EncoderIntel::encode(fcH264Frame& dst, const void *image, fcPixelFormat fmt, fcTime timestamp, bool force_keyframe)
{
    if (!isValid()) { return false; }

    // convert image to NV12
    AnyToNV12(m_nv12_image, m_rgba_image, image, fmt, m_conf.width, m_conf.height);
    NV12Data data = m_nv12_image.data();


    dst.timestamp = timestamp;

    mfxFrameSurface1 surface;
    memset(&surface, 0, sizeof(surface));
    surface.Info = m_params.mfx.FrameInfo;
    surface.Data.TimeStamp = (mfxU64)(timestamp * 90000.0); // unit is 90KHz
    surface.Data.MemType = MFX_MEMTYPE_SYSTEM_MEMORY;
    surface.Data.Y = (mfxU8*)data.y;
    surface.Data.UV = (mfxU8*)data.uv;
    surface.Data.V = (mfxU8*)data.uv + 1;
    surface.Data.Pitch = m_conf.width;
    surface.Data.FrameOrder = MFX_FRAMEORDER_UNKNOWN;

    mfxBitstream bitstream;
    memset(&bitstream, 0, sizeof(bitstream));
    bitstream.Data = (mfxU8*)m_encoded_data.data();
    bitstream.MaxLength = (mfxU32)m_encoded_data.size();

    // encode!
    mfxSyncPoint syncp;
    mfxStatus ret = m_encoder->EncodeFrameAsync(nullptr, &surface, &bitstream, &syncp);
    if (ret < 0) { return false; }

    // wait encode complete
    ret = m_session->SyncOperation(syncp, -1);
    if (ret < 0) { return false; }

    {
        // gather NAL information
        const static mfxU8 start_seq[] = { 0, 0, 1 }; // NAL start sequence
        mfxU8 *beg = bitstream.Data;
        mfxU8 *end = beg + bitstream.DataLength;
        for (;;) {
            auto *pos = std::search(beg, end, start_seq, start_seq + 3);
            if (pos == end) { break; }
            auto *next = std::search(pos + 1, end, start_seq, start_seq + 3);
            dst.nal_sizes.push_back(int(next - pos));
            beg = next;
        }
        dst.data.append((char*)bitstream.Data, bitstream.DataLength);
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

    return true;
}

fcIH264Encoder* fcCreateH264EncoderIntelHW(const fcH264EncoderConfig& conf)
{
    int impls[] = {
        MFX_IMPL_HARDWARE,
        MFX_IMPL_HARDWARE2,
        MFX_IMPL_HARDWARE3,
        MFX_IMPL_HARDWARE4,
    };

    for (int i : impls) {
        auto ret = new fcH264EncoderIntel(conf, i);
        if (ret->isValid()) {
            return ret;
        }
        else {
            delete ret;
        }
    }
    return nullptr;
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

fcIH264Encoder* fcCreateH264EncoderIntelHW(const fcH264EncoderConfig& conf) { return nullptr; }
fcIH264Encoder* fcCreateH264EncoderIntelSW(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportH264_Intel
