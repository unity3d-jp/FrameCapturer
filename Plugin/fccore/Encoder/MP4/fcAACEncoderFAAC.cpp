#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "fcMP4Internal.h"
#include "fcAACEncoder.h"

#ifdef fcSupportAAC_FAAC
#include <libfaac/faac.h>
#if defined(fcWindows)
    #define FAACDLL "libfaac.dll"
#elif defined(fcMac)
    #define FAACDLL "libfaac.dylib"
#elif defined(fcLinux)
    #define FAACDLL "libfaac.so"
#endif

class fcAACEncoderFAAC : public fcIAACEncoder
{
public:
    fcAACEncoderFAAC(const fcAACEncoderConfig& conf);
    ~fcAACEncoderFAAC() override;
    const char* getEncoderInfo() override;
    const Buffer& getDecoderSpecificInfo() override;
    bool encode(fcAACFrame& dst, const float *samples, size_t num_samples, fcTime timestamp) override;
    bool flush(fcAACFrame& dst) override;

    bool isValid() const { return m_handle != nullptr; }

private:
    fcAACEncoderConfig m_conf;
    void *m_handle = nullptr;
    unsigned long m_num_read_samples = 0;
    unsigned long m_output_size = 0;
    Buffer m_aac_tmp_buf;
    Buffer m_aac_header;
    RawVector<float> m_tmp_data;
    double m_time = 0.0;
};



using faacEncGetCurrentConfiguration_t = faacEncConfigurationPtr (FAACAPI*)(faacEncHandle hEncoder);
using faacEncSetConfiguration_t = int(FAACAPI*)(faacEncHandle hEncoder, faacEncConfigurationPtr config);
using faacEncOpen_t = faacEncHandle(FAACAPI*)(unsigned long sampleRate, unsigned int numChannels, unsigned long *inputSamples, unsigned long *maxOutputBytes);
using faacEncGetDecoderSpecificInfo_t = int(FAACAPI*)(faacEncHandle hEncoder, unsigned char **ppBuffer, unsigned long *pSizeOfDecoderSpecificInfo);
using faacEncEncode_t = int(FAACAPI*)(faacEncHandle hEncoder, int32_t * inputBuffer, unsigned int samplesInput, unsigned char *outputBuffer, unsigned int bufferSize);
using faacEncClose_t = int(FAACAPI*)(faacEncHandle hEncoder);

#define EachFAACFunctions(Body)\
    Body(faacEncGetCurrentConfiguration)\
    Body(faacEncSetConfiguration)\
    Body(faacEncOpen)\
    Body(faacEncGetDecoderSpecificInfo)\
    Body(faacEncEncode)\
    Body(faacEncClose)

#define Decl(name) static name##_t name##_;
    EachFAACFunctions(Decl)
#undef Decl

static module_t g_faac;


bool fcLoadFAACModule()
{
    if (g_faac != nullptr) { return true; }

    g_faac = DLLLoad(FAACDLL);
    if (g_faac == nullptr) { return false; }

    bool ok = true;
#define Imp(Name) (void*&)Name##_ = DLLGetSymbol(g_faac, #Name); if(!Name##_) { ok = false; }
    EachFAACFunctions(Imp)
#undef Imp

    if (!ok) {
        DLLUnload(g_faac);
        g_faac = nullptr;
    }

    return ok;
}

fcAACEncoderFAAC::fcAACEncoderFAAC(const fcAACEncoderConfig& conf)
    : m_conf(conf), m_handle(nullptr), m_num_read_samples(), m_output_size()
{
    m_handle = faacEncOpen_(conf.sample_rate, conf.num_channels, &m_num_read_samples, &m_output_size);

    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration_(m_handle);
    config->bitRate = conf.target_bitrate / conf.num_channels;
    config->quantqual = 100;
    config->inputFormat = FAAC_INPUT_FLOAT;
    config->mpegVersion = MPEG4;
    config->aacObjectType = LOW;
    config->allowMidside = 0;
    config->useLfe = 0;
    config->outputFormat = 1;
    faacEncSetConfiguration_(m_handle, config);
}

fcAACEncoderFAAC::~fcAACEncoderFAAC()
{
    faacEncClose_(m_handle);
    m_handle = nullptr;
}
const char* fcAACEncoderFAAC::getEncoderInfo() { return "FAAC"; }

bool fcAACEncoderFAAC::encode(fcAACFrame& dst, const float *samples, size_t num_samples, fcTime timestamp)
{
    m_aac_tmp_buf.resize(m_output_size);

    size_t pos = m_tmp_data.size();
    m_tmp_data.append(samples, num_samples);
    num_samples = m_tmp_data.size();
    for (size_t i = pos; i < num_samples; ++i) {
        m_tmp_data[i] *= 32767.0f;
    }
    samples = m_tmp_data.data();

    int total = 0;
    for (;;) {
        if (num_samples - total < m_num_read_samples) {
            m_tmp_data.erase(m_tmp_data.begin(), m_tmp_data.begin() + total);
            break;
        }

        int packet_size = faacEncEncode_(m_handle, (int32_t*)samples, m_num_read_samples, (unsigned char*)&m_aac_tmp_buf[0], m_output_size);
        if (packet_size > 0) {
            dst.data.append(m_aac_tmp_buf.data(), packet_size);

            double duration = (double)m_num_read_samples / (double)m_conf.sample_rate;
            dst.packets.push_back({ (uint32_t)packet_size, duration, m_time });
            m_time += duration;
        }
        total += m_num_read_samples;
        samples += m_num_read_samples;
    }
    return true;
}

bool fcAACEncoderFAAC::flush(fcAACFrame& dst)
{
    return false;
}

const Buffer& fcAACEncoderFAAC::getDecoderSpecificInfo()
{
    if (m_aac_header.empty()) {
        unsigned char *buf;
        unsigned long num_buf;
        faacEncGetDecoderSpecificInfo_(m_handle, &buf, &num_buf);
        m_aac_header.append((char*)buf, num_buf);
        //free(buf);
    }
    return m_aac_header;
}


fcIAACEncoder* fcCreateAACEncoderFAAC(const fcAACEncoderConfig& conf)
{
    if (!fcLoadFAACModule()) { return nullptr; }

    auto *ret = new fcAACEncoderFAAC(conf);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

#else  // fcSupportAAC_FAAC

bool fcLoadFAACModule() { return false; }
fcIAACEncoder* fcCreateAACEncoderFAAC(const fcAACEncoderConfig& conf) { return nullptr; }

#endif // fcSupportAAC_FAAC
