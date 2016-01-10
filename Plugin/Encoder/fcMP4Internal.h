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

struct fcMP4FrameData
{
    u64 timestamp;
    u64 offset;
    Buffer data;

    fcMP4FrameData() : timestamp(0), offset(0){}
};

typedef fcMP4FrameData fcH264Frame;
typedef fcMP4FrameData fcAACFrame;

struct fcVideoTrackSummary
{
    u32 duration;
    u32 width;
    u32 height;
};

struct fcAudioTrackSummary
{
    u32 duration;
    u32 unit_duration;
    u32 sample_rate;
    u32 bit_rate;
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
