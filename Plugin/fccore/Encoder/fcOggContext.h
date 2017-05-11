#pragma once

class fcIOggContext
{
public:
    virtual void release() = 0;
    virtual void addOutputStream(fcStream *s) = 0;
    virtual bool write(const float *samples, int num_samples, fcTime timestamp = -1.0) = 0;
protected:
    virtual ~fcIOggContext() {}
};
fcIOggContext* fcOggCreateContextImpl(const fcOggConfig *conf);
