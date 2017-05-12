#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "fcFlacContext.h"

#ifdef fcSupportFlac
#ifdef _MSC_VER
    #pragma comment(lib, "libFLAC_static.lib")
    #pragma comment(lib, "libvorbis_static.lib")
    #pragma comment(lib, "libogg_static.lib")
    #define FLAC__NO_DLL
#endif
#include "FLAC/stream_encoder.h"

class fcFlacWriter
{
public:
    fcFlacWriter(const fcFlacConfig& c, fcStream *s);
    ~fcFlacWriter();
    bool write(const int *samples, int num_samples);

private:
    fcStream *m_stream = nullptr;
    FLAC__StreamEncoder *m_encoder = nullptr;
};
using fcFlacWriterPtr = std::unique_ptr<fcFlacWriter>;


class fcFlacContext : public fcIFlacContext
{
public:
    using AudioBuffer = RawVector<float>;
    using AudioBufferPtr = std::shared_ptr<AudioBuffer>;
    using AudioBufferQueue = ResourceQueue<AudioBufferPtr>;

    fcFlacContext(const fcFlacConfig& c);
    ~fcFlacContext() override;
    void release() override;
    void addOutputStream(fcStream *s) override;
    bool write(const float *samples, int num_samples) override;

private:
    fcFlacConfig m_conf;
    std::vector<fcFlacWriterPtr> m_writers;

    TaskQueue           m_tasks;
    AudioBufferQueue    m_buffers;
    RawVector<int>      m_conversion_buffer;
};




static FLAC__StreamEncoderReadStatus stream_encoder_read_callback_(const FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    auto *f = (fcStream*)client_data;
    if (*bytes > 0) {
        *bytes = f->read(buffer, *bytes);
        if (*bytes == 0)
            return FLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM;
        else
            return FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE;
    }
    else
        return FLAC__STREAM_ENCODER_READ_STATUS_ABORT;
}

static FLAC__StreamEncoderWriteStatus stream_encoder_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
{
    auto *f = (fcStream*)client_data;
    if (f->write(buffer, bytes) != bytes)
        return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    else
        return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static FLAC__StreamEncoderSeekStatus stream_encoder_seekp_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
    auto *f = (fcStream*)client_data;
    f->seekp((size_t)absolute_byte_offset);
    return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

static FLAC__StreamEncoderTellStatus stream_encoder_tellp_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
    auto *f = (fcStream*)client_data;
    *absolute_byte_offset = f->tellp();
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}



fcFlacWriter::fcFlacWriter(const fcFlacConfig& c, fcStream *s)
{
    m_encoder = FLAC__stream_encoder_new();
    if (c.verify) {
        FLAC__stream_encoder_set_verify(m_encoder, true);
    }
    FLAC__stream_encoder_set_sample_rate(m_encoder, c.sample_rate);
    FLAC__stream_encoder_set_channels(m_encoder, c.num_channels);
    FLAC__stream_encoder_set_bits_per_sample(m_encoder, c.bits_per_sample);
    FLAC__stream_encoder_set_compression_level(m_encoder, c.compression_level);
    FLAC__stream_encoder_set_blocksize(m_encoder, c.block_size);
    FLAC__stream_encoder_init_stream(m_encoder, stream_encoder_write_callback_, stream_encoder_seekp_callback_, stream_encoder_tellp_callback_, /*metadata_callback=*/0, /*client_data=*/s);

#ifdef fcDebug
    {
        FLAC__uint64 absolute_sample;
        unsigned frame_number;
        unsigned channel;
        unsigned sample;
        FLAC__int32 expected;
        FLAC__int32 got;

        printf("testing FLAC__stream_encoder_get_verify_decoder_error_stats()... ");
        FLAC__stream_encoder_get_verify_decoder_error_stats(m_encoder, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);
        printf("OK\n");
    }
#endif // fcDebug
}

fcFlacWriter::~fcFlacWriter()
{
    if (m_encoder) {
        FLAC__stream_encoder_finish(m_encoder);
        FLAC__stream_encoder_delete(m_encoder);
        m_encoder = nullptr;
    }
}

bool fcFlacWriter::write(const int *samples, int num_samples)
{
    return FLAC__stream_encoder_process_interleaved(m_encoder, samples, num_samples) != 0;
}



fcFlacContext::fcFlacContext(const fcFlacConfig& c)
    : m_conf(c)
{
    for (int i = 0; i < 8; ++i) {
        m_buffers.push(AudioBufferPtr(new AudioBuffer()));
    }
}

fcFlacContext::~fcFlacContext()
{
    m_tasks.wait();
    m_writers.clear();
}

void fcFlacContext::release()
{
    delete this;
}

void fcFlacContext::addOutputStream(fcStream *s)
{
    if (s) {
        m_writers.emplace_back(new fcFlacWriter(m_conf, s));
    }
}

bool fcFlacContext::write(const float *samples, int num_samples)
{
    if (!samples || num_samples == 0) { return false; }

    auto buf = m_buffers.pop();
    buf->assign(samples, num_samples);

    m_tasks.run([this, buf]() {
        float scale = float((1 << (m_conf.bits_per_sample - 1)) - 1);
        m_conversion_buffer.resize(buf->size());
        fcF32ToI32Samples(m_conversion_buffer.data(), buf->data(), buf->size(), scale);
        for (auto& w : m_writers) {
            w->write(m_conversion_buffer.data(), (int)m_conversion_buffer.size() / m_conf.num_channels);
        }
        m_buffers.push(buf);
    });
    return true;
}

fcIFlacContext* fcFlacCreateContextImpl(const fcFlacConfig *conf)
{
    return new fcFlacContext(*conf);
}

#else // fcSupportFlac

fcIFlacContext* fcFlacCreateContextImpl(const fcFlacConfig *conf)
{
    return nullptr;
}

#endif // fcSupportFlac
