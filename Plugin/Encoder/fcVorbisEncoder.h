#pragma once


struct fcVorbisEncoderConfig
{
    int sample_rate;
    int num_channels;
    int target_bitrate;
};
using fcOpusEncoderConfig = fcVorbisEncoderConfig;

struct fcVorbisFrame
{
    struct Block
    {
        int size;
        uint64_t timestamp;
    };
    using Blocks = RawVector<Block>;

    Buffer data;
    Blocks blocks;

    void clear()
    {
        data.clear();
        blocks.clear();
    }

    // Body: [](const char *data, int size, uint64_t timestamp) {}
    template<class Body>
    void eachBlocks(const Body& body) const
    {
        int pos = 0;
        for (auto& s : blocks) {
            body(&data[pos], s.size, s.timestamp);
            pos += s.size;
        }
    }
};

class fcIVorbisEncoder
{
public:
    virtual ~fcIVorbisEncoder() {}
    virtual void release() = 0;
    virtual const char* getMatroskaCodecID() const = 0;
    virtual const Buffer& getCodecPrivate() const = 0;

    virtual bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples) = 0;
    virtual bool flush(fcVorbisFrame& dst) = 0;
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf);
fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf);

using fcWebMAudioEncoderConfig = fcVorbisEncoderConfig;
using fcWebMAudioFrame = fcVorbisFrame;
using fcIWebMAudioEncoder = fcIVorbisEncoder;
