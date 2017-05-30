#include "pch.h"
#include "fcInternal.h"
#include "Foundation/fcFoundation.h"
#include "fcOggContext.h"

#ifdef fcSupportVorbis
#ifdef _MSC_VER
    #pragma comment(lib, "libvorbis_static.lib")
    #pragma comment(lib, "libogg_static.lib")
#endif
#include "vorbis/vorbisenc.h"


class fcOggContext : public fcIOggContext
{
public:
    using AudioBuffer = RawVector<float>;
    using AudioBuffers = SharedResources<AudioBuffer>;

    fcOggContext(const fcOggConfig& conf);
    virtual ~fcOggContext() override;

    virtual void addOutputStream(fcStream *s) override;
    virtual bool addSamples(const float *samples, int num_samples) override;

    void pageOut();

private:
    fcOggConfig m_conf;
    std::vector<fcStream*> m_streams;

    TaskQueue           m_tasks;
    AudioBuffers        m_buffers;
    Buffer              m_header_data;

    vorbis_info         m_vo_info;
    vorbis_comment      m_vo_comment;
    vorbis_dsp_state    m_vo_dsp;
    vorbis_block        m_vo_block;

    ogg_stream_state    m_ogstream;
    ogg_page            m_ogpage;
};



fcOggContext::fcOggContext(const fcOggConfig& conf)
    : m_conf(conf)
{
    m_conf.max_tasks = std::max<int>(m_conf.max_tasks, 1);

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

    static int s_serial = 0;
    ogg_stream_init(&m_ogstream, ++s_serial);

    {
        ogg_packet og_header, og_header_comm, og_header_code;
        vorbis_analysis_headerout(&m_vo_dsp, &m_vo_comment, &og_header, &og_header_comm, &og_header_code);
        ogg_stream_packetin(&m_ogstream, &og_header);
        ogg_stream_packetin(&m_ogstream, &og_header_comm);
        ogg_stream_packetin(&m_ogstream, &og_header_code);

        // make ogg header data
        BufferStream hs(m_header_data);
        for (;;) {
            int result = ogg_stream_flush(&m_ogstream, &m_ogpage);
            if (result == 0) break;
            hs.write(m_ogpage.header, m_ogpage.header_len);
            hs.write(m_ogpage.body, m_ogpage.body_len);
        }
    }

    for (int i = 0; i < m_conf.max_tasks; ++i) {
        m_buffers.emplace();
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
    for (auto s : m_streams) { s->release(); }

    ogg_stream_clear(&m_ogstream);
    vorbis_block_clear(&m_vo_block);
    vorbis_dsp_clear(&m_vo_dsp);
    vorbis_comment_clear(&m_vo_comment);
    vorbis_info_clear(&m_vo_info);
}

void fcOggContext::addOutputStream(fcStream *s)
{
    if (!s) { return; }
    s->addRef();
    m_streams.push_back(s);
    s->write(m_header_data.data(), m_header_data.size());
}

bool fcOggContext::addSamples(const float *samples, int num_samples)
{
    if (!samples || num_samples == 0) { return false; }

    auto buf = m_buffers.acquire();
    buf->assign(samples, num_samples);

    m_tasks.run([this, buf]() {
        int num_channels = m_conf.num_channels;
        int num_samples = (int)buf->size();
        const float *samples = buf->data();

        int block_size = (int)num_samples / num_channels;
        float **buffer = vorbis_analysis_buffer(&m_vo_dsp, block_size);
        for (int bi = 0; bi < block_size; ++bi) {
            for (int ci = 0; ci < num_channels; ++ci) {
                buffer[ci][bi] = samples[bi*num_channels + ci];
            }
        }
        if (vorbis_analysis_wrote(&m_vo_dsp, block_size) == 0) {
            pageOut();
        }
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
            ogg_stream_packetin(&m_ogstream, &packet);
            for (;;) {
                int result = ogg_stream_pageout(&m_ogstream, &m_ogpage);
                if (result == 0) { break; }
                for (auto& w : m_streams) {
                    w->write(m_ogpage.header, m_ogpage.header_len);
                    w->write(m_ogpage.body, m_ogpage.body_len);
                }
                if (ogg_page_eos(&m_ogpage)) { break; }
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
