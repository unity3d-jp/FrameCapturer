#include "pch.h"
#include "fcFoundation.h"
#include "fcMP4Internal.h"
#include "fcAACEncoder.h"

#ifdef fcSupportFAAC

#include <libfaac/faac.h>
#define FAACDLL "libfaac" fcDLLExt

class fcFAACEncoder : public fcIAACEncoder
{
public:
    fcFAACEncoder(const fcAACEncoderConfig& conf);
    ~fcFAACEncoder() override;
    const char* getEncoderName() override;
    const Buffer& getEncoderInfo() override;
    bool encode(fcAACFrame& dst, const float *samples, size_t num_samples) override;

private:
    fcAACEncoderConfig m_conf;
    void *m_handle;
    unsigned long m_num_read_samples;
    unsigned long m_output_size;
    Buffer m_aac_tmp_buf;
    Buffer m_aac_header;
};


namespace {

    typedef faacEncConfigurationPtr
        (FAACAPI* faacEncGetCurrentConfiguration_t)(faacEncHandle hEncoder);


    typedef int(FAACAPI* faacEncSetConfiguration_t)(faacEncHandle hEncoder,
        faacEncConfigurationPtr config);


    typedef faacEncHandle(FAACAPI* faacEncOpen_t)(unsigned long sampleRate,
        unsigned int numChannels,
        unsigned long *inputSamples,
        unsigned long *maxOutputBytes);


    typedef int(FAACAPI* faacEncGetDecoderSpecificInfo_t)(faacEncHandle hEncoder, unsigned char **ppBuffer,
        unsigned long *pSizeOfDecoderSpecificInfo);


    typedef int(FAACAPI* faacEncEncode_t)(faacEncHandle hEncoder, int32_t * inputBuffer, unsigned int samplesInput,
        unsigned char *outputBuffer,
        unsigned int bufferSize);

    typedef int(FAACAPI* faacEncClose_t)(faacEncHandle hEncoder);

#define EachFAACFunctions(Body)\
    Body(faacEncGetCurrentConfiguration)\
    Body(faacEncSetConfiguration)\
    Body(faacEncOpen)\
    Body(faacEncGetDecoderSpecificInfo)\
    Body(faacEncEncode)\
    Body(faacEncClose)


#define decl(name) name##_t name##_i;
    EachFAACFunctions(decl)
#undef decl

module_t g_mod_faac;

} // namespace

fcFAACEncoder::fcFAACEncoder(const fcAACEncoderConfig& conf)
    : m_conf(conf), m_handle(nullptr), m_num_read_samples(), m_output_size()
{
    m_handle = faacEncOpen_i(conf.sampling_rate, conf.num_channels, &m_num_read_samples, &m_output_size);

    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration_i(m_handle);
    config->bitRate = conf.target_bitrate / conf.num_channels;
    config->quantqual = 100;
    config->inputFormat = FAAC_INPUT_FLOAT;
    config->mpegVersion = MPEG4;
    config->aacObjectType = LOW;
    config->allowMidside = 0;
    config->useLfe = 0;
    config->outputFormat = 1;
    faacEncSetConfiguration_i(m_handle, config);
}

fcFAACEncoder::~fcFAACEncoder()
{
    faacEncClose_i(m_handle);
    m_handle = nullptr;
}
const char* fcFAACEncoder::getEncoderName() { return "fcFAACEncoder"; }

bool fcFAACEncoder::encode(fcAACFrame& dst, const float *samples, size_t num_samples)
{
    m_aac_tmp_buf.resize(m_output_size);

    int block_index = 0;
    for (;;) {
        int process_size = std::min<int>((int)m_num_read_samples, (int)num_samples);
        int size_encoded = faacEncEncode_i(m_handle, (int32_t*)samples, process_size, (unsigned char*)&m_aac_tmp_buf[0], m_output_size);
        if (size_encoded > 0) {
            dst.data.append(&m_aac_tmp_buf[0], size_encoded);
            dst.encoded_block_sizes.push_back(size_encoded);
            dst.raw_block_sizes.push_back(process_size);
            ++block_index;
        }
        samples += process_size;
        num_samples -= process_size;
        if (num_samples <= 0) { break; }
    }
    return true;
}

const Buffer& fcFAACEncoder::getEncoderInfo()
{
    if (m_aac_header.empty()) {
        unsigned char *buf;
        unsigned long num_buf;
        faacEncGetDecoderSpecificInfo_i(m_handle, &buf, &num_buf);
        m_aac_header.append((char*)buf, num_buf);
        free(buf);
    }
    return m_aac_header;
}


bool fcLoadFAACModule()
{
    if (g_mod_faac != nullptr) { return true; }

    g_mod_faac = DLLLoad(FAACDLL);
    if (g_mod_faac == nullptr) { return false; }

#define imp(name) (void*&)name##_i = DLLGetSymbol(g_mod_faac, #name);
    EachFAACFunctions(imp)
#undef imp
    return true;
}


fcIAACEncoder* fcCreateFAACEncoder(const fcAACEncoderConfig& conf)
{
    if (!fcLoadFAACModule()) { return nullptr; }
    return new fcFAACEncoder(conf);
}

#else  // fcSupportFAAC

bool fcLoadFAACModule()
{
    return false;
}

fcIAACEncoder* fcCreateFAACEncoder(const fcAACEncoderConfig& conf)
{
    return nullptr;
}

#endif // fcSupportFAAC
