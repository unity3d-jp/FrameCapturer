#ifndef fcMP4Internal_h
#define fcMP4Internal_h

#include "Misc.h"
#include "Buffer.h"


// raw video sample
struct fcVideoFrame
{
    u64 timestamp;
    Buffer rgba;
    uint8_t *i420_y;
    uint8_t *i420_u;
    uint8_t *i420_v;

    fcVideoFrame() : timestamp(0), i420_y(nullptr), i420_u(nullptr), i420_v(nullptr) {}
    ~fcVideoFrame() { deallocate(); }

    void allocate(int width, int height)
    {
        deallocate();

        rgba.resize(width * height * 4);
        int af = roundup<2>(width) * roundup<2>(height);
        i420_y = (uint8_t*)AlignedAlloc(af, 32);
        i420_u = (uint8_t*)AlignedAlloc(af >> 2, 32);
        i420_v = (uint8_t*)AlignedAlloc(af >> 2, 32);
    }

    void deallocate()
    {
        rgba.clear();
        AlignedFree(i420_y);
        AlignedFree(i420_u);
        AlignedFree(i420_v);
    }
};

// raw audio sample
struct fcAudioFrame
{
    u64 timestamp;
    Buffer data;

    fcAudioFrame() : timestamp() {}
};

enum fcFrameType
{
    fcFrameType_Unknown,
    fcFrameType_H264,
    fcFrameType_AAC,
};

enum fcH264FrameType
{
    fcH264FrameType_Invalid,    ///< encoder not ready or parameters are invalidate
    fcH264FrameType_IDR,        ///< IDR frame in H.264
    fcH264FrameType_I,          ///< I frame type
    fcH264FrameType_P,          ///< P frame type
    fcH264FrameType_Skip,       ///< skip the frame based encoder kernel
    fcH264FrameType_IPMixed     ///< a frame where I and P slices are mixing, not supported yet
};

struct fcH264NALHeader
{
    uint8_t forbidden_zero_bit  : 1;
    uint8_t nal_ref_idc         : 2;
    uint8_t nal_unit_type       : 5; // ENalUnitType

    fcH264NALHeader() {}
    fcH264NALHeader(char c) {
        forbidden_zero_bit  = (c >> 7) & 0x01;
        nal_ref_idc         = (c >> 5) & 0x03;
        nal_unit_type       = c & 0x1f;
    }
};

struct fcFrameData
{
    fcFrameType type;
    uint64_t timestamp;
    Buffer data;

    fcFrameData() : type(fcFrameType_Unknown), timestamp(0){}
};

struct fcH264Frame : public fcFrameData
{
    fcH264FrameType h264_type;
    std::vector<int> nal_sizes;

    fcH264Frame() : h264_type(fcH264FrameType_Invalid) { type = fcFrameType_H264; }

    // Body: [](const char *nal_data, int nal_size) -> void
    template<class Body>
    void eachNALs(const Body& body)
    {
        int total = 0;
        for (int size : nal_sizes) {
            body(&data[total], size);
            total += size;
        }
    }
};

struct fcAACFrame : public fcFrameData
{
    fcAACFrame() { type = fcFrameType_AAC; }
};


struct fcFrameInfo
{
    const char *data;
    size_t size;
    uint64_t file_offset;
    uint64_t timestamp;
    uint32_t duration;
    uint32_t index;
    uint32_t index_track;
    fcFrameType type;

    fcFrameInfo()
        : data(), size(), file_offset(), timestamp(), duration(), index(), index_track(), type()
    {}
};

struct fcOffsetValue
{
    uint32_t count;
    uint32_t value;
};

struct fcSampleToChunk
{
    uint32_t first_chunk_ID;
    uint32_t samples_per_chunk;
};

#endif // fcMP4Internal_h
