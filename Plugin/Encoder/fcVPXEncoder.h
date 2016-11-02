#pragma once

struct fcVPXEncoderConfig
{
    int width = 0;
    int height = 0;
    int target_bitrate = 1024000;
    int target_framerate = 60;
};

struct fcVPXFrame
{
    struct PacketInfo
    {
        int size;
        double timestamp;
        uint32_t keyframe : 1;
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
        for (auto& p : packets) {
            body(&data[pos], p);
            pos += p.size;
        }
    }
};


class fcIVPXEncoder
{
public:
    virtual ~fcIVPXEncoder() {}
    virtual void release() = 0;
    virtual const char* getMatroskaCodecID() const = 0;

    virtual bool encode(fcVPXFrame& dst, const I420Data& image, fcTime timestamp, bool force_keyframe = false) = 0;
    virtual bool flush(fcVPXFrame& dst) = 0;
};

fcIVPXEncoder* fcCreateVP8Encoder(const fcVPXEncoderConfig& conf);
fcIVPXEncoder* fcCreateVP9Encoder(const fcVPXEncoderConfig& conf);

using fcWebMVideoEncoderConfig = fcVPXEncoderConfig;
using fcWebMVideoFrame = fcVPXFrame;
using fcIWebMVideoEncoder = fcIVPXEncoder;
