#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "fcOggContext.h"

#ifdef fcSupportVorbis
#include "vorbis/vorbisenc.h"
#include "fcVorbisEncoder.h"

class fcOggWriter
{
public:
    fcOggWriter(fcStream *s);
    ~fcOggWriter();
    void write(ogg_packet& packet);
    void flush();
    void pageOut();

private:
    fcStream *m_stream = nullptr;
    ogg_stream_state m_ogstream;
    ogg_page         m_ogpage;
    ogg_packet       m_ogpacket;

};
using fcOggWriterPtr = std::unique_ptr<fcOggWriter>;


class fcOggContext : public fcIOggContext
{
public:
    using AudioBuffer = RawVector<float>;
    using AudioBufferPtr = std::shared_ptr<AudioBuffer>;
    using AudioBufferQueue = ResourceQueue<AudioBufferPtr>;

    fcOggContext(const fcOggConfig& conf);
    virtual ~fcOggContext() override;
    virtual void release()  override;
    virtual void addOutputStream(fcStream *s) override;
    virtual bool write(const float *samples, int num_samples, fcTime timestamp) override;

    void pageOut();

private:
    fcOggConfig m_conf;
    std::vector<fcOggWriterPtr> m_writers;

    TaskQueue           m_tasks;
    AudioBufferQueue    m_buffers;

    vorbis_info         m_vo_info;
    vorbis_comment      m_vo_comment;
    vorbis_dsp_state    m_vo_dsp;
    vorbis_block        m_vo_block;
    ogg_packet          m_og_header;
    ogg_packet          m_og_header_comm;
    ogg_packet          m_og_header_code;
};



fcOggWriter::fcOggWriter(fcStream *s)
    : m_stream(s)
{
    static int s_serial = 0;
    ogg_stream_init(&m_ogstream, s_serial++);
}

fcOggWriter::~fcOggWriter()
{
    ogg_stream_clear(&m_ogstream);
}

void fcOggWriter::write(ogg_packet& packet)
{
    ogg_stream_packetin(&m_ogstream, &packet);
}

void fcOggWriter::flush()
{
    for (;;) {
        int result = ogg_stream_flush(&m_ogstream, &m_ogpage);
        if (result == 0)break;
        m_stream->write(m_ogpage.header, m_ogpage.header_len);
        m_stream->write(m_ogpage.body, m_ogpage.body_len);
    }
}

void fcOggWriter::pageOut()
{
    bool eos = false;
    while (!eos) {
        int result = ogg_stream_pageout(&m_ogstream, &m_ogpage);
        if (result == 0)break;
        m_stream->write(m_ogpage.header, m_ogpage.header_len);
        m_stream->write(m_ogpage.body, m_ogpage.body_len);

        if (ogg_page_eos(&m_ogpage))eos = true;
    }
}



fcOggContext::fcOggContext(const fcOggConfig& conf)
    : m_conf(conf)
{
    vorbis_info_init(&m_vo_info);
    switch (conf.bitrate_mode) {
    case fcBitrateMode::CBR:
        vorbis_encode_init(&m_vo_info, conf.num_channels, conf.sample_rate, conf.target_bitrate, conf.target_bitrate, conf.target_bitrate);
        break;
    case fcBitrateMode::VBR:
        vorbis_encode_init(&m_vo_info, conf.num_channels, conf.sample_rate, -1, conf.target_bitrate, -1);
        break;
    }
    vorbis_comment_init(&m_vo_comment);
    vorbis_analysis_init(&m_vo_dsp, &m_vo_info);
    vorbis_block_init(&m_vo_dsp, &m_vo_block);
    vorbis_analysis_headerout(&m_vo_dsp, &m_vo_comment, &m_og_header, &m_og_header_comm, &m_og_header_code);

    for (int i = 0; i < 8; ++i) {
        m_buffers.push(AudioBufferPtr(new AudioBuffer()));
    }
}


fcOggContext::~fcOggContext()
{
    m_tasks.run([this]() {
        if (vorbis_analysis_wrote(&m_vo_dsp, 0) == 0) {
            pageOut();
        }
    });
    m_tasks.wait();
    m_writers.clear();

    vorbis_block_clear(&m_vo_block);
    vorbis_dsp_clear(&m_vo_dsp);
    vorbis_comment_clear(&m_vo_comment);
    vorbis_info_clear(&m_vo_info);
}

void fcOggContext::release()
{
    delete this;
}

void fcOggContext::addOutputStream(fcStream *s)
{
    auto writer = new fcOggWriter(s);
    writer->write(m_og_header);
    writer->write(m_og_header_comm);
    writer->write(m_og_header_code);
    writer->flush();
    m_writers.emplace_back(writer);
}

bool fcOggContext::write(const float *samples, int num_samples, fcTime timestamp)
{
    if (!samples || num_samples == 0) { return false; }

    auto buf = m_buffers.pop();
    buf->assign(samples, num_samples);

    m_tasks.run([this, buf]() {
        int num_channels = m_conf.num_channels;
        int num_samples = (int)buf->size();
        const float *samples = buf->data();

        int block_size = (int)num_samples / num_channels;
        float **buffer = vorbis_analysis_buffer(&m_vo_dsp, block_size);
        for (int bi = 0; bi < block_size; bi += num_channels) {
            for (int ci = 0; ci < num_channels; ++ci) {
                buffer[ci][bi] = samples[bi*num_channels + ci];
            }
        }
        if (vorbis_analysis_wrote(&m_vo_dsp, block_size) == 0) {
            pageOut();
        }

        m_buffers.push(buf);
    });

    return true;
}

void fcOggContext::pageOut()
{
    while (vorbis_analysis_blockout(&m_vo_dsp, &m_vo_block) == 1) {
        vorbis_analysis(&m_vo_block, nullptr);
        vorbis_bitrate_addblock(&m_vo_block);

        ogg_packet packet;
        while (vorbis_bitrate_flushpacket(&m_vo_dsp, &packet) == 1) {
            for (auto& w : m_writers) {
                w->write(packet);
                w->pageOut();
            }
        }
    }
}


fcIOggContext* fcOggCreateContextImpl(const fcOggConfig *conf)
{
    return new fcOggContext(*conf);
}

#else // fcSupportVorbis

fcIOggContext* fcOggCreateContextImpl(const fcOggConfig *conf)
{
    return nullptr;
}

#endif // fcSupportVorbis
