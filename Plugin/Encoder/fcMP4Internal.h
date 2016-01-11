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
        i420_y = (uint8_t*)aligned_alloc(af, 32);
        i420_u = (uint8_t*)aligned_alloc(af >> 2, 32);
        i420_v = (uint8_t*)aligned_alloc(af >> 2, 32);
    }

    void deallocate()
    {
        rgba.clear();
        aligned_free(i420_y);
        aligned_free(i420_u);
        aligned_free(i420_v);
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

struct fcFrameData
{
    fcFrameType type;
    u64 timestamp;
    Buffer data;

    fcFrameData() : type(fcFrameType_Unknown), timestamp(0){}
};

struct fcH264Frame : public fcFrameData
{
    fcH264FrameType h264_type;

    fcH264Frame() : h264_type(fcH264FrameType_Invalid) { type = fcFrameType_H264; }
};

struct fcAACFrame : public fcFrameData
{
    fcAACFrame() { type = fcFrameType_AAC; }
};


struct fcFrameInfo
{
    const char *data;
    size_t size;
    u64 file_offset;
    u64 timestamp;
    u32 duration;
    u32 index;
    u32 index_track;
    fcFrameType type;

    fcFrameInfo()
        : data(), size(), file_offset(), timestamp(), duration(), index(), index_track(), type()
    {}
};

struct fcOffsetValue
{
    u32 count;
    u32 value;
};

struct fcSampleToChunk
{
    u32 first_chunk_ID;
    u32 samples_per_chunk;
};

u64 fcGetCurrentTimeNanosec();

#endif // fcMP4Internal_h
