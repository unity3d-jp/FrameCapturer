#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "fcFlacContext.h"

#ifdef fcSupportFlac

class fcFlacContext : public fcIFlacContext
{
public:
    ~fcFlacContext() override;
    void release() override;
    void addOutputStream(fcStream *s) override;
    bool write(const float *samples, int num_samples, fcTime timestamp) override;

private:
    void flacBegin(fcStream *s);
    void flacEnd(fcStream *s);

    fcGifConfig m_conf;
    std::vector<fcStream*> m_streams;
};


fcFlacContext::~fcFlacContext()
{
    for (auto s : m_streams) { flacEnd(s); }
}

void fcFlacContext::release()
{
    delete this;
}

void fcFlacContext::addOutputStream(fcStream *s)
{
    if (s) {
        flacBegin(s);
        m_streams.push_back(s);
    }
}

bool fcFlacContext::write(const float *samples, int num_samples, fcTime timestamp)
{
    // todo
    return true;
}

void fcFlacContext::flacBegin(fcStream *s)
{
    // todo
}

void fcFlacContext::flacEnd(fcStream *s)
{
    // todo
}

fcIFlacContext* fcFlacCreateContextImpl(const fcFlacConfig *conf)
{
    return new fcFlacContext();
}

#else // fcSupportFlac

fcIFlacContext* fcFlacCreateContextImpl(const fcFlacConfig *conf)
{
    return nullptr;
}

#endif // fcSupportFlac
