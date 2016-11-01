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
        int packet_size;
        int raw_data_size;
    };

    Buffer data;
    fcTime timestamp = 0.0;
    RawVector<PacketInfo> packets;

    void clear()
    {
        data.clear();
        packets.clear();
    }

    // Body: [](const char *data, int encoded_block_size, int raw_block_size) -> void
    template<class Body>
    void eachPackets(const Body& body) const
    {
        int total = 0;
        for (auto& p : packets) {
            body(&data[total], p.packet_size, p.raw_data_size);
            total += p.packet_size;
        }
    }
};


class fcIAACEncoder
{
public:
    virtual ~fcIAACEncoder() {}
    virtual const char* getEncoderInfo() = 0;
    virtual const Buffer& getDecoderSpecificInfo() = 0;
    virtual bool encode(fcAACFrame& dst, const float *samples, size_t num_samples) = 0;
};

bool fcDownloadFAAC(fcDownloadCallback cb);
bool fcLoadFAACModule();
fcIAACEncoder* fcCreateFAACEncoder(const fcAACEncoderConfig& conf);
