#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "fcWaveContext.h"

#ifdef fcSupportWave

class fcWaveContext : public fcIWaveContext
{
public:
    fcWaveContext(const fcWaveConfig& c);
    ~fcWaveContext() override;
    void release() override;
    void addOutputStream(fcStream *s) override;
    bool write(const float *samples, int num_samples, fcTime timestamp) override;

private:
    void waveBegin(fcStream *s);
    void waveEnd(fcStream *s);

    fcWaveConfig m_conf;
    std::vector<fcStream*> m_streams;
    Buffer m_sample_buffer;
    size_t m_sample_size = 0; // in byte
};

struct WaveHeader
{
    char    RIFFTag[4] = {'R', 'I', 'F', 'F'};
    int32_t nFileSize = 0;
    char    WAVETag[4] = { 'W', 'A', 'V', 'E' };
    char    fmtTag[4] = { 'f', 'm', 't', ' ' };
    int32_t nFmtSize = 16;
    int16_t shFmtID = 1;
    int16_t shCh = 2;
    int32_t nSampleRate = 48000;
    int32_t nBytePerSec = 96000;
    int16_t shBlockSize = 4;
    int16_t shBitPerSample = 16;
    char    dataTag[4] = { 'd', 'a', 't', 'a' };
    int32_t nBytesData = 0;
};

fcWaveContext::fcWaveContext(const fcWaveConfig& c)
    : m_conf(c)
{
}

fcWaveContext::~fcWaveContext()
{
    for (auto s : m_streams) { waveEnd(s); }
}

void fcWaveContext::release()
{
    delete this;
}

void fcWaveContext::addOutputStream(fcStream *s)
{
    if (s) {
        waveBegin(s);
        m_streams.push_back(s);
    }
}

void fcWaveContext::waveBegin(fcStream *s)
{
    WaveHeader header;
    header.nSampleRate = m_conf.sample_rate;
    header.shCh = m_conf.num_channels;
    header.shBitPerSample = m_conf.bits_per_sample;
    header.nBytePerSec = header.nSampleRate * header.shBitPerSample * header.shCh / 8;
    header.shBlockSize = header.shBitPerSample * header.shCh / 8;
    s->write(&header, sizeof(header));
}

void fcWaveContext::waveEnd(fcStream *s)
{
    uint32_t total_size = (uint32_t)(m_sample_size + sizeof(WaveHeader));
    uint32_t filesize = total_size - 8;
    uint32_t datasize = total_size - 44;
    s->seekp(4);
    s->write(&filesize, 4);
    s->seekp(40);
    s->write(&datasize, 4);
}

bool fcWaveContext::write(const float *samples, int num_samples, fcTime timestamp)
{
    if (m_conf.bits_per_sample == 8) {
        m_sample_buffer.resize(num_samples * 1);
        fcF32ToU8Samples((uint8_t*)m_sample_buffer.data(), samples, num_samples);
    }
    else if (m_conf.bits_per_sample == 16) {
        m_sample_buffer.resize(num_samples * 2);
        fcF32ToI16Samples((int16_t*)m_sample_buffer.data(), samples, num_samples);
    }
    else if (m_conf.bits_per_sample == 24) {
        m_sample_buffer.resize(num_samples * 3);
        fcF32ToI24Samples((uint8_t*)m_sample_buffer.data(), samples, num_samples);
    }
    m_sample_size += m_sample_buffer.size();

    for (auto s : m_streams) {
        s->write(m_sample_buffer.data(), m_sample_buffer.size());
    }
    return true;
}


fcIWaveContext* fcWaveCreateContextImpl(const fcWaveConfig *conf)
{
    return new fcWaveContext(*conf);
}

#else // fcSupportWave

fcIWaveContext* fcWaveCreateContextImpl(const fcWaveConfig *conf)
{
    return nullptr;
}

#endif // fcSupportWave
