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
    struct PacketInfo
    {
        uint32_t size;
        double duration;
        double timestamp;
    };
    using Packets = RawVector<PacketInfo>;

    Buffer data;
    Packets packets;

    void clear()
    {
        data.clear();
        packets.clear();
    }

    // Body: [](const char *data, const PacketInfo& pinfo) {}
    template<class Body>
    void eachPackets(const Body& body) const
    {
        int pos = 0;
        for (auto& s : packets) {
            body(&data[pos], s);
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

    virtual bool encode(fcVorbisFrame& dst, const float *samples, size_t num_samples, fcTime timestamp) = 0;
    virtual bool flush(fcVorbisFrame& dst) = 0;
};


fcIVorbisEncoder* fcCreateVorbisEncoder(const fcVorbisEncoderConfig& conf);
fcIVorbisEncoder* fcCreateOpusEncoder(const fcOpusEncoderConfig& conf);

using fcWebMAudioEncoderConfig = fcVorbisEncoderConfig;
using fcWebMAudioFrame = fcVorbisFrame;
using fcIWebMAudioEncoder = fcIVorbisEncoder;
