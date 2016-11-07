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
    fcH264EncoderIntel(const fcH264EncoderConfig& conf);
    ~fcH264EncoderIntel() override;
    const char* getEncoderInfo() override;
    bool encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe) override;

    bool isValid() { return m_encoder != nullptr; }

private:
    fcH264EncoderConfig m_conf;
    std::unique_ptr<MFXVideoSession> m_session;
    std::unique_ptr<MFXVideoENCODE> m_encoder;
    mfxVideoParam m_params;
    mfxFrameSurface1 m_surface;
    mfxBitstream m_bitstream;
    mfxSyncPoint m_sync;
    Buffer m_tmp_outdata;
};



fcH264EncoderIntel::fcH264EncoderIntel(const fcH264EncoderConfig& conf)
    : m_conf(conf)
{
    mfxStatus ret;
    m_session.reset(new MFXVideoSession());
    ret = m_session->Init(MFX_IMPL_AUTO_ANY, nullptr);
    if (ret < 0) {
        m_session.reset();
        return;
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
    params.mfx.CodecProfile = MFX_PROFILE_AVC_BASELINE;
    params.mfx.GopOptFlag = MFX_GOP_CLOSED;
    params.mfx.GopPicSize = 0;
    params.mfx.GopRefDist = 0;
    params.mfx.TargetUsage = MFX_TARGETUSAGE_BEST_SPEED;

    params.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
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


    memset(&m_surface, 0, sizeof(m_surface));
    memset(&m_bitstream, 0, sizeof(m_bitstream));
    m_tmp_outdata.resize(params.mfx.BufferSizeInKB * 1024);
    m_bitstream.Data = (mfxU8*)m_tmp_outdata.data();
    m_bitstream.MaxLength = (mfxU32)m_tmp_outdata.size();
}

fcH264EncoderIntel::~fcH264EncoderIntel()
{
    m_encoder.reset();
    m_session.reset();
}

const char* fcH264EncoderIntel::getEncoderInfo() { return "Intel H264 Encoder"; }

bool fcH264EncoderIntel::encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe)
{
    if (!isValid()) { return false; }

    dst.timestamp = timestamp;

    m_surface.Info = m_params.mfx.FrameInfo;
    m_surface.Data.TimeStamp = (mfxU64)(timestamp * 90000.0); // unit is 90KHz
    m_surface.Data.MemType = MFX_MEMTYPE_SYSTEM_MEMORY;
    m_surface.Data.Y = (mfxU8*)data.y;
    m_surface.Data.U = (mfxU8*)data.u;
    m_surface.Data.V = (mfxU8*)data.v;
    m_surface.Data.Pitch = m_conf.width;
    m_surface.Data.FrameOrder = MFX_FRAMEORDER_UNKNOWN;

    mfxStatus ret = m_encoder->EncodeFrameAsync(nullptr, &m_surface, &m_bitstream, &m_sync);
    if (ret < 0) { return false; }

    ret = m_session->SyncOperation(m_sync, -1);
    if (ret < 0) { return false; }

    const static mfxU8 start_seq[] = { 0, 0, 1 };
    mfxU8 *beg = m_bitstream.Data;
    mfxU8 *end = beg + m_bitstream.DataLength;
    for (;;) {
        auto *pos = std::search(beg, end, start_seq, start_seq + 3);
        if (pos == end) { break; }
        auto *next = std::search(pos + 1, end, start_seq, start_seq + 3);

        fcH264NALHeader header(pos[3]);
        dst.nal_sizes.push_back(next - pos);
        beg = next;
    }
    dst.data.append((char*)m_bitstream.Data, m_bitstream.DataLength);
    m_bitstream.DataLength = 0;

    return true;
}

fcIH264Encoder* fcCreateH264EncoderIntel(const fcH264EncoderConfig& conf)
{
    auto ret = new fcH264EncoderIntel(conf);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

#else // fcSupportH264_Intel

fcIH264Encoder* fcCreateH264EncoderIntel(const fcH264EncoderConfig& conf) { return nullptr; }

#endif // fcSupportH264_Intel
