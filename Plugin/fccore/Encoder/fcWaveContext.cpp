#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "fcWaveContext.h"

#ifdef fcSupportWave

class fcWaveContext : public fcIWaveContext
{
public:
    ~fcWaveContext() override;
    void release() override;
    void addOutputStream(fcStream *s) override;
    bool write(const float *samples, int num_samples, fcTime timestamp) override;

private:
    void waveBegin(fcStream *s);
    void waveEnd(fcStream *s);

    fcGifConfig m_conf;
    std::vector<fcStream*> m_streams;
};


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
    // todo
}

void fcWaveContext::waveEnd(fcStream *s)
{
    // todo
}

bool fcWaveContext::write(const float *samples, int num_samples, fcTime timestamp)
{
    // todo
    return true;
}


fcIWaveContext* fcWaveCreateContextImpl(const fcWaveConfig *conf)
{
    return new fcWaveContext();
}

#else // fcSupportWave

fcIWaveContext* fcWaveCreateContextImpl(const fcWaveConfig *conf)
{
    return nullptr;
}

#endif // fcSupportWave
