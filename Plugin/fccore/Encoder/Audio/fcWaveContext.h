#pragma once

class fcIWaveContext : public fcContextBase
{
public:
    virtual void addOutputStream(fcStream *s) = 0;
    virtual bool write(const float *samples, int num_samples) = 0;
};
fcIWaveContext* fcWaveCreateContextImpl(const fcWaveConfig *conf);
