#include "pch.h"
#include <libfaac/faac.h>
#include "fcFoundation.h"
#include "fcAACEncoder.h"


#if defined(fcWindows)
    #define FAACDLL "libfaac.dll"
#elif defined(fcMac)
    #define FAACDLL "libfaac.dylib"
#elif
    #define FAACDLL "libfaac.so"
#endif

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

#define decl(name) name##_t name##_imp;
decl(faacEncGetCurrentConfiguration)
decl(faacEncSetConfiguration)
decl(faacEncOpen)
decl(faacEncGetDecoderSpecificInfo)
decl(faacEncEncode)
decl(faacEncClose)
#undef decl

module_t g_mod_faac;

static bool LoadFAACModule()
{
    if (g_mod_faac != nullptr) { return true; }

    g_mod_faac = module_load(FAACDLL);
    if (g_mod_faac == nullptr) { return false; }

#define imp(name) (void*&)name##_imp = module_getsymbol(g_mod_faac, #name);
imp(faacEncGetCurrentConfiguration)
imp(faacEncSetConfiguration)
imp(faacEncOpen)
imp(faacEncGetDecoderSpecificInfo)
imp(faacEncEncode)
imp(faacEncClose)
#undef imp
    return true;
}

} // namespace



bool fcAACEncoder::loadModule()
{
    return LoadFAACModule();
}

fcAACEncoder::fcAACEncoder(int sampling_rate, int num_channels, int bitrate)
    : m_handle(nullptr), m_num_read_samples(), m_output_size()
{
    if (!loadModule()) { return; }

    m_handle = faacEncOpen_imp(sampling_rate, num_channels, &m_num_read_samples, &m_output_size);

    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration_imp(m_handle);
    config->bitRate = bitrate / num_channels;
    config->quantqual = 100;
    config->inputFormat = FAAC_INPUT_FLOAT;
    config->mpegVersion = MPEG4;
    config->aacObjectType = LOW;
    config->allowMidside = 0;
    config->useLfe = 0;
    config->outputFormat = 1;
    int ret = faacEncSetConfiguration_imp(m_handle, config);
}

fcAACEncoder::~fcAACEncoder()
{
    if (!loadModule()) { return; }

    faacEncClose_imp(m_handle);
    m_handle = nullptr;
}

fcAACEncoder::operator bool() const
{
    return loadModule();
}

fcAACEncoder::Result fcAACEncoder::encode(const float *samples, int num_samples)
{
    if (!loadModule()) { return Result(); }

    m_aac_buf.clear();
    m_aac_tmp_buf.resize(m_output_size);
    for (;;) {
        int process_size = std::min<int>(m_num_read_samples, num_samples);
        int size_encoded = faacEncEncode_imp(m_handle, (int32_t*)samples, process_size, (unsigned char*)&m_aac_tmp_buf[0], m_output_size);
        m_aac_buf.append(&m_aac_tmp_buf[0], size_encoded);
        samples += process_size;
        num_samples -= process_size;
        if (num_samples <= 0) { break; }
    }
    return Result(&m_aac_buf[0], m_aac_buf.size());
}

const std::string& fcAACEncoder::getHeader()
{
    unsigned char *buf;
    unsigned long num_buf;
    faacEncGetDecoderSpecificInfo_imp(m_handle, &buf, &num_buf);
    m_aac_header.assign((char*)buf);
    free(buf);
    return m_aac_header;
}
