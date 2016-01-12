#include "pch.h"
#include "fcMP4Internal.h"
#include "fcMP4StreamWriter.h"

namespace {

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


time_t GetMacTime()
{
    return time(0) + 2082844800;
}

} // namespace


fcMP4StreamWriter::fcMP4StreamWriter(BinaryStream& stream, const fcMP4Config &conf)
    : m_stream(stream)
    , m_conf(conf)
{
    mp4Begin();
}

fcMP4StreamWriter::~fcMP4StreamWriter()
{
    mp4End();
}

void fcMP4StreamWriter::mp4Begin()
{
    m_frame_info.reserve(60 * 60);

    BinaryStream& os = m_stream;
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

    m_mdat_begin = os.tellp();

    os  << u32_be(0x1)
        << u32_be('mdat')
        << u64(0); // reserve mdat size space
}

void fcMP4StreamWriter::addFrame(const fcFrameData& frame)
{
    BinaryStream& os = m_stream;

    fcFrameInfo info;
    if (frame.type == fcFrameType_H264) {
        const auto& h264 = (const fcH264Frame&)frame;

        if (h264.data[0] == 0x17 && h264.data[1] == 0) { //if SPS/PPS
            u8 *pos = (u8*)&h264.data[11];
            int sps_size = u16_be(*(u16*)pos);

            m_sps.assign(pos + 2, pos + 2 + sps_size);
            os << u16(0);
            os.write(pos, sps_size+2);

            pos += m_sps.size() + 3;
            int pps_size = u16_be(*(u16*)pos);

            m_pps.assign(pos + 2, pos + 2 + pps_size);
            os << u16(0);
            os.write(pos, pps_size + 2);

            info.size = sps_size + pps_size + 8;
        }
        else {
            // todo: handling SEI

            info.size += h264.data.size() - 5;
            os.write(&h264.data[5], info.size);
        }
    }
    else if (frame.type == fcFrameType_AAC) {

        info.size = frame.data.size() - 2;
        info.data = &frame.data[2];
        os.write(info.data, info.size);
    }


    m_frame_info.emplace_back(info);
}

void fcMP4StreamWriter::mp4End()
{
    if (m_frame_info.empty()) {
        fcDebugLog("fcMP4Stream::mp4End() no frame data.");
        return;
    }

    const std::string version_string = "MP4 Capturer by Unity Technologies Japan";
    const std::string audio_track_name = "Sound Media Handler";
    const std::string video_track_name = "Video Media Handler";
    const std::string video_compression_name = "AVC Coding";
    u32 mac_time = (u32)GetMacTime();

    size_t num_video_frames = 0;
    size_t num_audio_frames = 0;
    u32 video_unit_duration = 1000;
    u32 audio_unit_duration = 1000;
    u64 video_duration = 0;
    u64 audio_duration = 0;
    fcMP4Config& c = m_conf;

    // compute file offset
    {
        const u64 mp4_header_size = 54;
        u64 last_size = 0;
        u64 last_offset = 0;
        u32 i = 0;
        eachFrame([&](fcFrameInfo& v) {
            v.index = i++;
            v.file_offset = last_offset + last_size + mp4_header_size;
            last_offset += last_size;
            last_size = v.size;
        });
    }

    // compute duration
    {
        fcFrameInfo *first_frame = nullptr;
        fcFrameInfo *last_frame = nullptr;
        u32 i = 0;

        auto proc = [&](fcFrameInfo& v) {
            v.index_track = i++;
            if (first_frame == nullptr) {
                first_frame = &v;
            }
            else if(last_frame != nullptr) {
                last_frame->duration = v.timestamp - last_frame->timestamp;
            }
            last_frame = &v;
        };

        eachVideoFrame(proc);
        num_video_frames = i;
        if(i > 0) video_duration = first_frame->timestamp - last_frame->timestamp;

        first_frame = last_frame = nullptr;
        i = 0;

        eachAudioFrame(proc);
        num_audio_frames = i;
        if (i > 0) audio_duration = first_frame->timestamp - last_frame->timestamp;
    }


    bool has_video = num_video_frames != 0;
    bool has_audio = num_audio_frames != 0;

    if (!has_video) {
        fcDebugLog("fcMP4Stream::flush() no video frames.");
        return;
    }




    BinaryStream& os = m_stream;
    size_t mdat_end = os.tellp();




    Buffer dd_buf; // decoder descriptor
    BufferStream dd(dd_buf);
    if (has_audio) {
        Buffer add_buf; //  audio decoder descriptor
        BufferStream add(add_buf); // audio decoder descriptor
        add << u8(64)
            << u8(0x15)         // stream/type flags.  always 0x15 for my purposes.
            << u8(0)            // buffer size, just set it to 1536 for both mp3 and aac
            << u16_be(0x600)
            << u32_be(c.audio_bitrate) // max bit rate (cue bill 'o reily meme for these two)
            << u32_be(c.audio_bitrate) // avg bit rate
            << u8(0x5)          //decoder specific descriptor type
            << u8(m_audio_header.data.size() - 2);
        add.write(&m_audio_header.data[2], m_audio_header.data.size() - 2);

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
    track_info.reserve( (num_video_frames + num_audio_frames) * 20 + 131072);
    BufferStream bs(track_info);
    Box box = Box(bs);



    box(u32_be('moov'), [&]() {

        //------------------------------------------------------
        // header
        //------------------------------------------------------
        box(u32_be('mvhd'), [&]() {
            bs << u32(0);                       // version and flags (none)
            bs << u32_be(mac_time);             // creation time
            bs << u32_be(mac_time);             // modified time
            bs << u32_be(video_unit_duration);  // time base (milliseconds = 1000)
            bs << u32_be(video_duration);       // duration (in time base units)
            bs << u32_be(0x00010000);           // fixed point playback speed 1.0
            bs << u16_be(0x0100);               // fixed point vol 1.0
            bs << u64(0);                       // reserved (10 bytes)
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
        //------------------------------------------------------
        if (has_audio) {
            box(u32_be('trak'), [&]() {
                box(u32_be('tkhd'), [&]() {
                    bs << u32_be(0x00000007);   // version (0) and flags (0xF)
                    bs << u32_be(mac_time);     // creation time
                    bs << u32_be(mac_time);     // modified time
                    bs << u32_be(1);            // track ID
                    bs << u32(0);               // reserved
                    bs << u32_be(audio_duration);// duration (in time base units)
                    bs << u64(0);               // reserved
                    bs << u16(0);               // video layer (0)
                    bs << u16_be(0);            // quicktime alternate track id
                    bs << u16_be(0x0100);       // volume
                    bs << u16(0);               // reserved
                    bs << u32_be(0x00010000) << u32_be(0x00000000) << u32_be(0x00000000); // window matrix row 1 (1.0, 0.0, 0.0)
                    bs << u32_be(0x00000000) << u32_be(0x00010000) << u32_be(0x00000000); // window matrix row 2 (0.0, 1.0, 0.0)
                    bs << u32_be(0x00000000) << u32_be(0x00000000) << u32_be(0x40000000); // window matrix row 3 (0.0, 0.0, 16384.0)
                    bs << u32(0);               // video width (fixed point)
                    bs << u32(0);               // video height (fixed point)
                });
                box(u32_be('mdia'), [&]() {
                    box(u32_be('mdhd'), [&]() {
                        bs << u32(0);                       // version and flags (none)
                        bs << u32_be(mac_time);             // creation time
                        bs << u32_be(mac_time);             // modified time
                        bs << u32_be(c.audio_sample_rate);  // time scale
                        bs << u32_be(audio_unit_duration);
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
                                    bs << u32_be(c.audio_sample_rate << 16); // sample rate (fixed point)
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
                                bs << u32_be(num_audio_frames);
                                eachAudioFrame([&](fcFrameInfo& v) {
                                    bs << u32_be(v.size);
                                });
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
        //------------------------------------------------------
        box(u32_be('trak'), [&]() {
            box(u32_be('tkhd'), [&]() {
                bs << u32_be(0x00000007);  // version (0) and flags (0x7)
                bs << u32_be(mac_time);            // creation time
                bs << u32_be(mac_time);            // modified time
                bs << u32_be(2);           // track ID
                bs << u32(0);                   // reserved
                bs << u32_be(video_duration);      // duration (in time base units)
                bs << u64(0);                   // reserved
                bs << u16(0);                   // video layer (0)
                bs << u16(0);                   // quicktime alternate track id (0)
                bs << u16(0);                   // track audio volume (this is video, so 0)
                bs << u16(0);                   // reserved
                bs << u32_be(0x00010000) << u32_be(0x00000000) << u32_be(0x00000000); //window matrix row 1 (1.0, 0.0, 0.0)
                bs << u32_be(0x00000000) << u32_be(0x00010000) << u32_be(0x00000000); //window matrix row 2 (0.0, 1.0, 0.0)
                bs << u32_be(0x00000000) << u32_be(0x00000000) << u32_be(0x40000000); //window matrix row 3 (0.0, 0.0, 16384.0)
                bs << u32_be(c.video_width << 16);  // video width (fixed point)
                bs << u32_be(c.video_height << 16); // video height (fixed point)
            }); // tkhd

            box(u32_be('mdia'), [&]() {
                box(u32_be('mdhd'), [&]() {
                    bs << u32(0);           // version and flags (none)
                    bs << u32_be(mac_time);    // creation time
                    bs << u32_be(mac_time);    // modified time
                    bs << u32_be(video_unit_duration);     // time scale
                    bs << u32_be(video_duration);
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
                                bs << u16_be(c.video_width);    // video_width
                                bs << u16_be(c.video_height);   // video_height
                                bs << u32_be(0x00480000);   // fixed point video_width pixel resolution (72.0)
                                bs << u32_be(0x00480000);   // fixed point video_height pixel resolution (72.0)
                                bs << u32(0);               // quicktime video data size 
                                bs << u16_be(1);            // frame count(?)
                                bs << u8(video_compression_name.size()); // compression name length
                                bs.write(video_compression_name.c_str(), video_compression_name.size()); // 31 bytes for the name
                                bs << u16(u16_be(24));      // bit depth
                                bs << u16(0xFFFF);          // quicktime video color table id (none = -1)
                                box(u32_be('avcC'), [&]() {
                                    bs << u8(1);            // version
                                    bs << u8(100);          // h264 profile ID
                                    bs << u8(0);            // h264 compatible profiles
                                    bs << u8(0x1f);         // h264 level
                                    bs << u8(0xff);         // reserved
                                    bs << u8(0xe1);         // first half-byte = no clue. second half = sps count
                                    bs << u16_be(m_sps.size()); // sps size
                                    bs.write(&m_sps[0], m_sps.size()); // sps data
                                    bs << u8(1); // pps count
                                    bs << u16_be(m_pps.size()); // pps size
                                    bs.write(&m_pps[0], m_pps.size()); // pps data
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
                            bs << u32_be(num_video_frames);
                            eachVideoFrame([&](fcFrameInfo& v) {
                                bs << u32_be(v.size);
                            });
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
        //------------------------------------------------------
        // todo: remove this if possible
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


    u64 mdat_size = u64_be(mdat_end - m_mdat_begin);

    os.write(track_info.ptr(), track_info.size());
    os.seekp(m_mdat_begin);
    os.write(&mdat_size, sizeof(u64));

    fcDebugLog("fcMP4Stream::mp4End() done.");
}
