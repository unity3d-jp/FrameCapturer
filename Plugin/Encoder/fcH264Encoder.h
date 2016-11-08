#pragma once


struct fcH264EncoderConfig
{
    int width;
    int height;
    int target_framerate;
    fcBitrateMode bitrate_mode;
    int target_bitrate;
};


enum fcH264FrameType
{
    fcH264FrameType_UNKNOWN = 0,
    fcH264FrameType_I       = 0x01,
    fcH264FrameType_P       = 0x02,
    fcH264FrameType_B       = 0x04,
    fcH264FrameType_S       = 0x08,
    fcH264FrameType_REF     = 0x10,
    fcH264FrameType_IDR     = 0x20,
};

enum fcH264NALType
{
    fcH264NALType_UNKNOWN = 0,
    fcH264NALType_SLICE = 1,
    fcH264NALType_SLICE_DPA = 2,
    fcH264NALType_SLICE_DPB = 3,
    fcH264NALType_SLICE_DPC = 4,
    fcH264NALType_SLICE_IDR = 5,
    fcH264NALType_SEI = 6,
    fcH264NALType_SPS = 7,
    fcH264NALType_PPS = 8,
    fcH264NALType_AUD = 9,
    fcH264NALType_FILLER = 12,
};

enum fcH264NALPriority
{
    fcH264NALPriority_DISPOSABLE = 0,
    fcH264NALPriority_LOW = 1,
    fcH264NALPriority_HIGH = 2,
    fcH264NALPriority_HIGHEST = 3,
};

struct fcH264NALHeader
{
    uint8_t type : 5; // fcH264NALType
    uint8_t ref_idc : 2;
    uint8_t forbidden_zero_bit : 1;

    fcH264NALHeader() {}
    fcH264NALHeader(char c) {
        forbidden_zero_bit = (c >> 7) & 0x01;
        ref_idc = (c >> 5) & 0x03;
        type = c & 0x1f;
    }
};


struct fcH264Frame
{
    Buffer data;
    double timestamp = 0;
    int type = 0; // combination of fcH264FrameType
    RawVector<int> nal_sizes;

    void clear()
    {
        timestamp = 0;
        type = 0;
        data.clear();
        nal_sizes.clear();
    }

    // Body: [](const char *nal_data, int nal_size) -> void
    template<class Body>
    void eachNALs(const Body& body) const
    {
        int total = 0;
        for (int size : nal_sizes) {
            body(&data[total], size);
            total += size;
        }
    }
};


class fcIH264Encoder
{
public:
    virtual ~fcIH264Encoder() {}
    virtual const char* getEncoderInfo() = 0;
    virtual bool encode(fcH264Frame& dst, const I420Data& data, fcTime timestamp, bool force_keyframe = false) = 0;
};

bool fcDownloadOpenH264(fcDownloadCallback cb);
bool fcLoadOpenH264Module();

fcIH264Encoder* fcCreateH264EncoderOpenH264(const fcH264EncoderConfig& conf);
fcIH264Encoder* fcCreateH264EncoderNVIDIA(const fcH264EncoderConfig& conf);
fcIH264Encoder* fcCreateH264EncoderAMD(const fcH264EncoderConfig& conf);
fcIH264Encoder* fcCreateH264EncoderIntelHW(const fcH264EncoderConfig& conf);
fcIH264Encoder* fcCreateH264EncoderIntelSW(const fcH264EncoderConfig& conf);
