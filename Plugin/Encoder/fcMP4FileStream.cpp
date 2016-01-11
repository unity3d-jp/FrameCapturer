#include "pch.h"
#include "fcMP4Internal.h"


class fcMP4Stream
{
public:
    fcMP4Stream(std::iostream &stream, const fcVideoTrackSummary &vts, const fcAudioTrackSummary &ats);
    virtual ~fcMP4Stream();
    void addFrame(const fcFrameData& buf);
    void flush();

private:
    void mp4Begin();
    void mp4End();

private:
    std::iostream& m_stream;
    fcVideoTrackSummary m_vts;
    fcAudioTrackSummary m_ats;
    std::list<fcFrameData> m_frames;

    std::vector<fcFrameData*> m_video_frames;
    std::vector<fcFrameData*> m_audio_frames;
    std::vector<fcSampleToChunk> m_video_samples_to_chunk;
    std::vector<fcSampleToChunk> m_audio_sample_to_chunk;
    std::vector<fcOffsetValue> m_video_decode_times;
    std::vector<fcOffsetValue> m_audio_decode_times;
    std::vector<fcOffsetValue> m_composition_offsets;
    std::vector<u64> m_video_chunks;
    std::vector<u64> m_audio_chunks;
    std::vector<u32> m_iframe_ids;
    size_t m_mdat_begin;
};

fcMP4Stream::fcMP4Stream(std::iostream& stream, const fcVideoTrackSummary &vts, const fcAudioTrackSummary &ats)
    : m_stream(stream)
    , m_vts(vts)
    , m_ats(ats)
{
    mp4Begin();
}

fcMP4Stream::~fcMP4Stream()
{
    mp4End();
}


class Box
{
public:
    Box(BinaryStream& stream) : m_stream(stream) {}

    template<class F>
    void operator()(u32 name, const F &f)
    {
        size_t offset = m_stream.tellp();
        m_stream << u32(0) << name; // reserve

        f();

        size_t pos = m_stream.tellp();
        u32 box_size = (u32)(pos - offset);
        m_stream.seekp(offset);
        m_stream << u32_be(box_size);
        m_stream.seekp(pos);
    }

private:
    BinaryStream& m_stream;
};


time_t fcGetMacTime()
{
    return time(0) + 2082844800;
}



void fcMP4Stream::mp4Begin()
{
    if (!m_stream) {
        fcDebugLog("fcMP4Stream::mp4Begin() invalid stream.");
        return;
    }
}

void fcMP4Stream::addFrame(const fcFrameData& frame)
{
    m_frames.emplace_back(frame);
}

void fcMP4Stream::mp4End()
{
    if (!m_stream) {
        fcDebugLog("fcMP4Stream::mp4End() invalid stream.");
        return;
    }
    if (m_frames.empty()) {
        fcDebugLog("fcMP4Stream::mp4End() no frame data.");
        return;
    }


    u64 first_timestamp = m_frames.front().timestamp;
    for (auto& v : m_frames) {
        if (v.type == fcFrameType_H264) {
            m_video_frames.push_back(&v);
        }
        else if (v.type == fcFrameType_AAC) {
            m_audio_frames.push_back(&v);
        }
    }

    std::vector<size_t> offsets;
    {
        size_t last_size = 0;
        size_t last_offset = 0;
        for (auto& v : m_frames) {
            offsets.push_back(last_offset + last_size);
            last_offset += last_size;
            last_size = v.data.size();
        }
    }



    if (m_video_frames.empty()) {
        fcDebugLog("fcMP4Stream::flush() no video frames.");
        return;
    }
    bool has_audio = !m_audio_frames.empty();

    size_t mdat_begin = 0;
    size_t mdat_end = 0;
    StdIOStream os = StdIOStream(m_stream);

    os  << u32_be(0x20)
        << u32_be('ftyp')
        << u32_be('isom')
        << u32_be(0x200)
        << u32_be('isom')
        << u32_be('iso2')
        << u32_be('avc1') 
        << u32_be('mp41')
        << u32_be(0x8)
        << u32_be('free');

    mdat_begin = os.tellp();

    os  << u32_be(0x1)
        << u32_be('mdat')
        << u64(0); // reserve mdat size space

    // write all frame data
    for (auto& v : m_frames) {
        // todo: 
        //os.write(v.data.ptr(), v.data.size());
        if (v.type == fcFrameType_H264) {
        }
        else if (v.type == fcFrameType_AAC) {
        }
    }

    mdat_end = os.tellp();




    Buffer dd_buf; // decoder descriptor
    BufferStream dd(dd_buf);
    if (!m_audio_frames.empty()) {
        const auto &audio_header = *m_audio_frames.front();
        ;   // todo

        Buffer add_buf; //  audio decoder descriptor
        BufferStream add(add_buf); // audio decoder descriptor
        add << u8(64)
            << u8(0x15)         // stream/type flags.  always 0x15 for my purposes.
            << u8(0)            // buffer size, just set it to 1536 for both mp3 and aac
            << u16_be(0x600)
            << u32_be(m_ats.bit_rate) // max bit rate (cue bill 'o reily meme for these two)
            << u32_be(m_ats.bit_rate) // avg bit rate
            << u8(0x5)          //decoder specific descriptor type
            << u8(audio_header.data.size() - 2);
        add.write(&audio_header.data[2], audio_header.data.size() - 2);

        dd << u16(0);   // es id
        dd << u8(0);    // stream priority
        dd << u8(4);    // descriptor type
        dd << u8(add_buf.size());
        dd.write(add_buf.ptr(), add_buf.size());
        dd << u8(0x6);  // config descriptor type
        dd << u8(1);    // len
        dd << u8(2);    // SL value(? always 2)
    }



    //-------------------------------------------

    Buffer track_info;
    track_info.reserve( (m_video_frames.size() + m_audio_frames.size()) * 20 + 131072);
    BufferStream bs(track_info);
    Box box = Box(bs);

    const std::string version_string = "MP4 Capturer by Unity Technologies Japan";
    const std::string audio_track_name = "Sound Media Handler";
    const std::string video_track_name = "Video Media Handler";
    const char video_compression_name[32] = "AVC Coding";
    // todo: fill these
    u32 mac_time = (u32)fcGetMacTime();


    std::vector<u8> sps, pps;
    {
        u8 *header = (u8*)&m_video_frames.front()->data[11];
        size_t len = u16_be(*(u16*)header);
        sps.assign(header + 2, header + 2 + len);

        header += sps.size() + 3;
        len = u16_be(*(u16*)header);
        pps.assign(header + 2, header + 2 + len);
    }

    box(u32_be('moov'), [&]() {

        //------------------------------------------------------
        // header
        box(u32_be('mvhd'), [&]() {
            bs << u32(0);               // version and flags (none)
            bs << u32(mac_time);        // creation time
            bs << u32(mac_time);        // modified time
            bs << u32_be(1000);         // time base (milliseconds, so 1000)
            bs << u32(m_vts.duration);  // duration (in time base units)
            bs << u32_be(0x00010000);   // fixed point playback speed 1.0
            bs << u16_be(0x0100);       // fixed point vol 1.0
            bs << u64(0);               // reserved (10 bytes)
            bs << u16(0);
            bs << u32_be(0x00010000) << u32_be(0x00000000) << u32_be(0x00000000); // window matrix row 1 (1.0, 0.0, 0.0)
            bs << u32_be(0x00000000) << u32_be(0x00010000) << u32_be(0x00000000); // window matrix row 2 (0.0, 1.0, 0.0)
            bs << u32_be(0x00000000) << u32_be(0x00000000) << u32_be(0x40000000); // window matrix row 3 (0.0, 0.0, 16384.0)
            bs << u32(0);   // prevew start time (time base units)
            bs << u32(0);   // prevew duration (time base units)
            bs << u32(0);   // still poster frame (timestamp of frame)
            bs << u32(0);   // selection(?) start time (time base units)
            bs << u32(0);   // selection(?) duration (time base units)
            bs << u32(0);   // current time (0, time base units)
            bs << u32_be(has_audio ? 3 : 2);// next free track id (1-based rather than 0-based)
        });

        //------------------------------------------------------
        // audio track
        if (has_audio) {
            box(u32_be('trak'), [&]() {
                box(u32_be('tkhd'), [&]() {
                    bs << u32_be(0x00000007); // version (0) and flags (0xF)
                    bs << u32(mac_time);      // creation time
                    bs << u32(mac_time);      // modified time
                    bs << u32_be(1);          // track ID
                    bs << u32(0);             // reserved
                    bs << u32(m_ats.duration);// duration (in time base units)
                    bs << u64(0);             // reserved
                    bs << u16(0);             // video layer (0)
                    bs << u16_be(0);          // quicktime alternate track id
                    bs << u16_be(0x0100);     // volume
                    bs << u16(0);             // reserved
                    bs << u32_be(0x00010000) << u32_be(0x00000000) << u32_be(0x00000000); // window matrix row 1 (1.0, 0.0, 0.0)
                    bs << u32_be(0x00000000) << u32_be(0x00010000) << u32_be(0x00000000); // window matrix row 2 (0.0, 1.0, 0.0)
                    bs << u32_be(0x00000000) << u32_be(0x00000000) << u32_be(0x40000000); // window matrix row 3 (0.0, 0.0, 16384.0)
                    bs << u32(0);             // video width (fixed point)
                    bs << u32(0);             // video height (fixed point)
                });
                box(u32_be('mdia'), [&]() {
                    box(u32_be('mdhd'), [&]() {
                        bs << u32(0);                   // version and flags (none)
                        bs << u32(mac_time);            // creation time
                        bs << u32(mac_time);            // modified time
                        bs << u32_be(m_ats.sample_rate);// time scale
                        bs << u32(m_ats.unit_duration);
                        bs << u32_be(0x15c70000);
                    }); // mdhd
                    box(u32_be('hdlr'), [&]() {
                        bs << u32(0);           // version and flags (none)
                        bs << u32(0);           // quicktime type (none)
                        bs << u32_be('soun');   // media type
                        bs << u32(0);           // manufacturer reserved
                        bs << u32(0);           // quicktime component reserved flags
                        bs << u32(0);           // quicktime component reserved mask
                        bs.write(audio_track_name.c_str(), audio_track_name.size() + 1); //track name
                    }); // hdlr
                    box(u32_be('minf'), [&]() {
                        box(u32_be('smhd'), [&]() {
                            bs << u32(0); // version and flags (none)
                            bs << u32(0); // balance (fixed point)
                        });
                        box(u32_be('dinf'), [&]() {
                            box(u32_be('dref'), [&]() {
                                bs << u32(0);       // version and flags (none)
                                bs << u32_be(1);    // count
                                box(u32_be('url '), [&]() {
                                    bs << u32_be(0x00000001); // version (0) and flags (1)
                                }); // url
                            }); // dref
                        }); // dinf
                        box(u32_be('stbl'), [&]() {
                            box(u32_be('stsd'), [&]() {
                                bs << u32(0);       //version and flags (none)
                                bs << u32_be(1);    //count
                                box(u32_be('mp4a'), [&]() {
                                    bs << u32(0);       // reserved (6 bytes)
                                    bs << u16(0);
                                    bs << u16_be(1);    // dref index
                                    bs << u16(0);       // quicktime encoding version
                                    bs << u16(0);       // quicktime encoding revision
                                    bs << u32(0);       // quicktime audio encoding vendor
                                    bs << u16(0);       // channels (ignored)
                                    bs << u16_be(16);   // sample size
                                    bs << u16(0);       // quicktime audio compression id
                                    bs << u16(0);       // quicktime audio packet size
                                    bs << u32_be(m_ats.sample_rate << 16); // sample rate (fixed point)
                                    box(u32_be('esds'), [&]() {
                                        bs << u32(0);   // version and flags (none)
                                        bs << u8(3);    // ES descriptor type
                                        bs << u8(dd_buf.size());
                                        bs.write(dd_buf.ptr(), dd_buf.size());
                                    }); // esds
                                }); // mp4a
                            }); // stsd

                            box(u32_be('stts'), [&]() {
                                bs << u32(0);   // version and flags (none)
                                bs << u32_be(m_audio_decode_times.size());
                                for (auto& v : m_audio_decode_times) {
                                    bs << u32_be(v.count) << u32_be(v.value);
                                }
                            });

                            box(u32_be('stsc'), [&]() {
                                bs << u32(0);   // version and flags (none)
                                bs << u32_be(m_audio_sample_to_chunk.size());
                                for (auto& v : m_audio_sample_to_chunk) {
                                    bs << u32_be(v.first_chunk_ID) << u32_be(v.samples_per_chunk) << u32(u32_be(1));
                                }
                            });

                            box(u32_be('stsz'), [&]() {
                                bs << u32(0);   // version and flags (none)
                                bs << u32(0);   // block size for all (0 if differing sizes)
                                bs << u32_be(m_audio_frames.size());
                                for (auto &v : m_audio_frames) {
                                    bs << u32_be(v->data.size());
                                }
                            });

                            if (!m_audio_chunks.empty() && m_audio_chunks.back() > 0xFFFFFFFFLL)
                            {
                                box(u32_be('co64'), [&]() {
                                    bs << u32(0); // version and flags (none)
                                    bs << u32_be(m_audio_chunks.size());
                                    for (auto &v : m_audio_chunks) {
                                        bs << u64_be(v);
                                    }
                                });
                            }
                            else
                            {
                                box(u32_be('stco'), [&]() {
                                    bs << u32(0); // version and flags (none)
                                    bs << u32_be(m_audio_chunks.size());
                                    for (auto &v : m_audio_chunks) {
                                        bs << u32_be(v);
                                    }
                                });
                            }
                        }); // stbl
                    }); // minf
                }); // mdia
            }); // trak
        }

        //------------------------------------------------------
        // video track
        box(u32_be('trak'), [&]() {
            box(u32_be('tkhd'), [&]() {
                bs << u32(u32_be(0x00000007));  // version (0) and flags (0x7)
                bs << u32(mac_time);            // creation time
                bs << u32(mac_time);            // modified time
                bs << u32(u32_be(2));           // track ID
                bs << u32(0);                   // reserved
                bs << u32(m_vts.duration);        // duration (in time base units)
                bs << u64(0);                   // reserved
                bs << u16(0);                   // video layer (0)
                bs << u16(0);                   // quicktime alternate track id (0)
                bs << u16(0);                   // track audio volume (this is video, so 0)
                bs << u16(0);                   // reserved
                bs << u32_be(0x00010000) << u32_be(0x00000000) << u32_be(0x00000000); //window matrix row 1 (1.0, 0.0, 0.0)
                bs << u32_be(0x00000000) << u32_be(0x00010000) << u32_be(0x00000000); //window matrix row 2 (0.0, 1.0, 0.0)
                bs << u32_be(0x00000000) << u32_be(0x00000000) << u32_be(0x40000000); //window matrix row 3 (0.0, 0.0, 16384.0)
                bs << u32_be(m_vts.width << 16);  // video width (fixed point)
                bs << u32_be(m_vts.height << 16); // video height (fixed point)
            }); // tkhd

            box(u32_be('mdia'), [&]() {
                box(u32_be('mdhd'), [&]() {
                    bs << u32(0);           // version and flags (none)
                    bs << u32(mac_time);    // creation time
                    bs << u32(mac_time);    // modified time
                    bs << u32_be(1000);     // time scale
                    bs << u32(m_vts.duration);
                    bs << u32_be(0x55c40000);
                }); // mdhd
                box(u32_be('hdlr'), [&]() {
                    bs << u32(0);           // version and flags (none)
                    bs << u32(0);           // quicktime type (none)
                    bs << u32_be('vide');   // media type
                    bs << u32(0);           // manufacturer reserved
                    bs << u32(0);           // quicktime component reserved flags
                    bs << u32(0);           // quicktime component reserved mask
                    bs.write(video_track_name.c_str(), video_track_name.size() + 1); //track name
                }); // hdlr
                box(u32_be('minf'), [&]() {
                    box(u32_be('vmhd'), [&]() {
                        bs << u32_be(0x00000001); //version (0) and flags (1)
                        bs << u16(0);       // quickdraw graphic mode (copy = 0)
                        bs << u16(0);       // quickdraw red value
                        bs << u16(0);       // quickdraw green value
                        bs << u16(0);       // quickdraw blue value
                    }); // 
                    box(u32_be('dinf'), [&]() {
                        box(u32_be('dref'), [&]() {
                            bs << u32(0); //version and flags (none)
                            bs << u32_be(1); //count
                            box(u32_be('url '), [&]() {
                                bs << u32_be(0x00000001); //version (0) and flags (1)
                            }); // dref
                        }); // dref
                    }); // dinf

                    box(u32_be('stbl'), [&]() {
                        box(u32_be('stsd'), [&]() {
                            bs << u32(0);       // version and flags (none)
                            bs << u32_be(1);    // count
                            box(u32_be('avc1'), [&]() {
                                bs << u32(0);               //reserved 6 bytes
                                bs << u16(0);
                                bs << u16_be(1);            // index
                                bs << u16(0);               // encoding version
                                bs << u16(0);               // encoding revision level
                                bs << u32(0);               // encoding vendor
                                bs << u32(0);               // temporal quality
                                bs << u32(0);               // spatial quality
                                bs << u16_be(m_vts.width);    // video_width
                                bs << u16_be(m_vts.height);   // video_height
                                bs << u32_be(0x00480000);   // fixed point video_width pixel resolution (72.0)
                                bs << u32_be(0x00480000);   // fixed point video_height pixel resolution (72.0)
                                bs << u32(0);               // quicktime video data size 
                                bs << u16_be(1);            // frame count(?)
                                bs << u8(strlen(video_compression_name)); // compression name length
                                bs.write(video_compression_name, sizeof(video_compression_name)); // 31 bytes for the name
                                bs << u16(u16_be(24));      // bit depth
                                bs << u16(0xFFFF);          // quicktime video color table id (none = -1)
                                box(u32_be('avcC'), [&]() {
                                    bs << u8(1);            // version
                                    bs << u8(100);          // h264 profile ID
                                    bs << u8(0);            // h264 compatible profiles
                                    bs << u8(0x1f);         // h264 level
                                    bs << u8(0xff);         // reserved
                                    bs << u8(0xe1);         // first half-byte = no clue. second half = sps count
                                    bs << u16_be(sps.size()); // sps size
                                    bs.write(&sps[0], sps.size()); // sps data
                                    bs << u8(1); // pps count
                                    bs << u16_be(pps.size()); // pps size
                                    bs.write(&pps[0], pps.size()); // pps data
                                }); // 
                            }); // avc1
                        }); // stsd

                        box(u32_be('stts'), [&]() {
                            bs << u32(0); // version and flags (none)
                            bs << u32_be(m_video_decode_times.size());
                            for (auto& v : m_video_decode_times)
                            {
                                bs << u32_be(v.count);
                                bs << u32_be(v.value);
                            }
                        }); // stts

                        if (m_iframe_ids.size())
                        {
                            box(u32_be('stss'), [&]() {
                                bs << u32(0); // version and flags (none)
                                bs << u32_be(m_iframe_ids.size());
                                bs.write(&m_iframe_ids[0], m_iframe_ids.size()*sizeof(u32));
                            }); // stss
                        }

                        box(u32_be('ctts'), [&]() {
                            bs << u32(0); // version (0) and flags (none)
                                          // bs << u32(u32_be(0x01000000)); // version (1) and flags (none)
                            bs << u32_be(m_composition_offsets.size());
                            for (auto& v : m_composition_offsets)
                            {
                                bs << u32_be(v.count);
                                bs << u32_be(v.value);
                            }
                        }); // ctts

                        box(u32_be('stsc'), [&]() {
                            bs << u32(0); // version and flags (none)
                            bs << u32_be(m_video_samples_to_chunk.size());
                            for (auto& v : m_video_samples_to_chunk)
                            {
                                bs << u32_be(v.first_chunk_ID);
                                bs << u32_be(v.samples_per_chunk);
                                bs << u32_be(1);
                            }
                        }); // stsc

                        box(u32_be('stsz'), [&]() {
                            bs << u32(0); // version and flags (none)
                            bs << u32(0); // block size for all (0 if differing sizes)
                            bs << u32_be(m_video_frames.size());
                            for (auto& v : m_video_frames) {
                                bs << u32_be(v->data.size());
                            }
                        }); // stsz

                        if (!m_video_chunks.empty() && m_video_chunks.back() > 0xFFFFFFFFLL)
                        {
                            box(u32_be('co64'), [&]() {
                                bs << u32(0); // version and flags (none)
                                bs << u32_be(m_video_chunks.size());
                                for (auto& v : m_video_chunks) {
                                    bs << u64_be(v);
                                }
                            }); // co64
                        }
                        else
                        {
                            box(u32_be('stco'), [&]() {
                                bs << u32(0); // version and flags (none)
                                bs << u32_be(m_video_chunks.size());
                                for (auto& v : m_video_chunks) {
                                    bs << u32_be(v);
                                }
                            }); // stco
                        }


                    }); // stbl
                }); // minf
            }); // mdia
        }); // trak

        
        //------------------------------------------------------
        // info
        box(u32_be('udta'), [&]() {
            box(u32_be('meta'), [&]() {
                bs << u32(0);       // version and flags (none)
                box(u32_be('hdlr'), [&]() {
                    bs << u32(0);           // version and flags (none)
                    bs << u32(0);           // quicktime type
                    bs << u32_be('mdir');   // metadata type
                    bs << u32_be('appl');   // quicktime manufacturer reserved thingy
                    bs << u32(0);           // quicktime component reserved flag
                    bs << u32(0);           // quicktime component reserved flag mask
                    bs << u8(0);            // null string
                }); // hdlr

                box(u32_be('ilst'), [&]() {
                    box(u32_be('\xa9too'), [&]() {
                        box(u32_be('data'), [&]() {
                            bs << u32_be(1);    // version (1) + flags (0)
                            bs << u32(0);       // reserved
                            bs.write(version_string.c_str(), version_string.size());
                        }); // data
                    }); // @too
                }); // ilst
            }); // meta
        }); // udta

    }); // moov


    u64 mdat_size = u64_be(mdat_end - mdat_begin);

    os.write(track_info.ptr(), track_info.size());
    os.seekp(mdat_begin);
    os.write((char*)&mdat_size, sizeof(u64));

    fcDebugLog("fcMP4Stream::mp4End() done.");
}
