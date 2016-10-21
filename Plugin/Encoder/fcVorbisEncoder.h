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
    struct Segment
    {
        int size;
        uint64_t timestamp;
    };
    using Segments = RawVector<Segment>;

    Buffer data;
    Segments segments;

    void clear()
    {
        data.clear();
        segments.clear();
    }

    // Body: [](const char *data, int size, uint64_t timestamp) {}
    template<class Body>
    void eachSegments(const Body& body) const
    {
        int pos = 0;
        for (auto& s : segments) {
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
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf);
fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf);

using fcWebMAudioEncoderConfig = fcVorbisEncoderConfig;
using fcWebMAudioFrame = fcVorbisFrame;
using fcIWebMAudioEncoder = fcIVorbisEncoder;
