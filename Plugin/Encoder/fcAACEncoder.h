#pragma once


struct fcAACEncoderConfig
{
    int sample_rate;
    int num_channels;
    int target_bitrate;

    fcAACEncoderConfig() : sample_rate(), num_channels(), target_bitrate() {}
};


struct fcAACFrame
{
    struct PacketInfo
    {
        uint32_t size;
        double duration;
        double timestamp;
    };

    Buffer data;
    RawVector<PacketInfo> packets;

    void clear()
    {
        data.clear();
        packets.clear();
    }

    // Body: [](const char *data, const PacketInfo& pinfo) -> void
    template<class Body>
    void eachPackets(const Body& body) const
    {
        int total = 0;
        for (auto& p : packets) {
            body(&data[total], p);
            total += p.size;
        }
    }
};


class fcIAACEncoder
{
public:
    virtual ~fcIAACEncoder() {}
    virtual const char* getEncoderInfo() = 0;
    virtual const Buffer& getDecoderSpecificInfo() = 0;
    virtual bool encode(fcAACFrame& dst, const float *samples, size_t num_samples, fcTime timestamp) = 0;
};

bool fcDownloadFAAC(fcDownloadCallback cb);
bool fcLoadFAACModule();
fcIAACEncoder* fcCreateFAACEncoder(const fcAACEncoderConfig& conf);
