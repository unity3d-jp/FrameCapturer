#include "pch.h"
#include "Misc.h"
#include "Buffer.h"



class fcMP4Stream
{
public:
    fcMP4Stream();
    virtual ~fcMP4Stream();
    void setStream(std::ostream *os);
    void addVideoBuffer(const Buffer &buf);
    void addAudioBuffer(const Buffer &buf);
    void flush();


protected:
    size_t getPosition();
    template<class F> void box(u32 name, const F &f);

private:
    std::unique_ptr<std::ostream> m_os;
    StreamBuffer m_boxes;
    std::list<Buffer> m_video;
    std::list<Buffer> m_audio;
};

fcMP4Stream::fcMP4Stream()
{

}

fcMP4Stream::~fcMP4Stream()
{
    flush();
}

void fcMP4Stream::setStream(std::ostream *os)
{
    m_os.reset(os);
}

size_t fcMP4Stream::getPosition()
{
    return m_os->tellp();
}

void fcMP4Stream::addVideoBuffer(const Buffer &buf)
{
    m_video.push_back(buf);
}

void fcMP4Stream::addAudioBuffer(const Buffer &buf)
{
    m_audio.push_back(buf);
}

template<class F>
void fcMP4Stream::box(u32 name, const F &f)
{
    size_t offset = getPosition();
    m_boxes << u32(0) << name;

    f();

    u32 box_size = (u32)(GetPos() - offset);
}


void fcMP4Stream::flush()
{
    if (!m_os || (m_video.empty() && m_audio.empty())) { return; }

    auto& os = *m_os;
    auto& bs = m_boxes;

    os << u32_be(0x20)
        << u32_be('ftyp') << u32_be('isom') << u32_be(0x200) << u32_be('isom') << u32_be('iso2') << u32_be('avc1') << u32_be('mp41')
        << u32_be(0x8) << u32_be('free');

    auto data_pos = getPosition();

    os << u32_be(0x1) << u32_be('mdat')
        // mark 64bit
        << u64(0);

    if (!m_audio.empty()) {
        const auto &audio_header = m_audio.front();
        u32 bit_rate;   // todo

        bs << u8(64)
            << u8(0x15) //stream/type flags.  always 0x15 for my purposes.
            << u8(0) //buffer size, just set it to 1536 for both mp3 and aac
            << u16_be(0x600)
            << u32_be(bit_rate) //max bit rate (cue bill 'o reily meme for these two)
            << u32_be(bit_rate) //avg bit rate
            << u8(0x5) //decoder specific descriptor type
            << u8(audio_header.size() - 2);
        bs.write(&audio_header[2], audio_header.size() - 2);
    }


#if 0

//        List<BYTE> esDescriptor;
//        BufferOutputSerializer esOut(esDescriptor);
//        esOut.u16(0); //es id
//        esOut.OutputByte(0); //stream priority
//        esOut.OutputByte(4); //descriptor type
//        /*esOut.OutputByte(0x80); //some stuff that no one should probably care about
//        esOut.OutputByte(0x80);
//        esOut.OutputByte(0x80);*/
//        esOut.OutputByte(esDecoderDescriptor.Num());
//        esOut.Serialize((LPVOID)esDecoderDescriptor.Array(), esDecoderDescriptor.Num());
//        esOut.OutputByte(0x6);  //config descriptor type
//        /*esOut.OutputByte(0x80); //some stuff that no one should probably care about
//        esOut.OutputByte(0x80);
//        esOut.OutputByte(0x80);*/
//        esOut.OutputByte(1); //len
//        esOut.OutputByte(2); //SL value(? always 2)
//
//        //-------------------------------------------
//
    box(u32_be('moov'), [&]() {
        //------------------------------------------------------
        // header
        box(u32_be('mvhd'), [&]() {
            bs << u32(0); //version and flags (none)
            bs << u32(macTime); //creation time
            bs << u32(macTime); //modified time
            bs << u32_be(1000); //time base (milliseconds, so 1000)
            bs << u32(videoDuration); //duration (in time base units)
            bs << u32_be(0x00010000); //fixed point playback speed 1.0
            bs << u16_be(0x0100); //fixed point vol 1.0
            bs << u64(0); //reserved (10 bytes)
            bs << u16(0);
            bs << u32_be(0x00010000) << u32_be(0x00000000) << u32_be(0x00000000); //window matrix row 1 (1.0, 0.0, 0.0)
            bs << u32_be(0x00000000) << u32_be(0x00010000) << u32_be(0x00000000); //window matrix row 2 (0.0, 1.0, 0.0)
            bs << u32_be(0x00000000) << u32_be(0x00000000) << u32_be(0x40000000); //window matrix row 3 (0.0, 0.0, 16384.0)
            bs << u32(0); //prevew start time (time base units)
            bs << u32(0); //prevew duration (time base units)
            bs << u32(0); //still poster frame (timestamp of frame)
            bs << u32(0); //selection(?) start time (time base units)
            bs << u32(0); //selection(?) duration (time base units)
            bs << u32(0); //current time (0, time base units)
            bs << u32_be(3); //next free track id (1-based rather than 0-based)
        });

        //------------------------------------------------------
        // audio track
        box(u32_be('trak'), [&]() {
            box(u32_be('tkhd'), [&]() {
              bs << u32_be(0x00000007); //version (0) and flags (0xF)
              bs << u32(macTime); //creation time
              bs << u32(macTime); //modified time
              bs << u32_be(1); //track ID
              bs << u32(0); //reserved
              bs << u32(audioDuration); //duration (in time base units)
              bs << u64(0); //reserved
              bs << u16(0); //video layer (0)
              bs << u16_be(0); //quicktime alternate track id
              bs << u16_be(0x0100); //volume
              bs << u16(0); //reserved
              bs << u32_be(0x00010000) << u32_be(0x00000000) << u32_be(0x00000000); //window matrix row 1 (1.0, 0.0, 0.0)
              bs << u32_be(0x00000000) << u32_be(0x00010000) << u32_be(0x00000000); //window matrix row 2 (0.0, 1.0, 0.0)
              bs << u32_be(0x00000000) << u32_be(0x00000000) << u32_be(0x40000000); //window matrix row 3 (0.0, 0.0, 16384.0)
              bs << u32(0); //width (fixed point)
              bs << u32(0); //height (fixed point)
            });
            box(u32_be('mdia'), [&]() {
                box(u32_be('mdhd'), [&]() {
                    bs << u32(0); //version and flags (none)
                    bs << u32(macTime); //creation time
                    bs << u32(macTime); //modified time
                    bs << u32_be(sampleRateHz); //time scale
                    bs << u32(audioUnitDuration);
                    bs << u32_be(0x15c70000);
                }); // mdhd
                box(u32_be('hdlr'), [&]() {
                    bs << u32(0); //version and flags (none)
                    bs << u32(0); //quicktime type (none)
                    bs << u32_be('soun'); //media type
                    bs << u32(0); //manufacturer reserved
                    bs << u32(0); //quicktime component reserved flags
                    bs << u32(0); //quicktime component reserved mask
                    bs.write((LPVOID)lpAudioTrack, (DWORD)strlen(lpAudioTrack) + 1); //track name
                }); // hdlr
                box(u32_be('minf'), [&]() {
                    box(u32_be('smhd'), [&]() {
                        bs << u32(0); //version and flags (none)
                        bs << u32(0); //balance (fixed point)
                    });
                    box(u32_be('dinf'), [&]() {
                        box(u32_be('dref'), [&]() {
                            bs << u32(0); //version and flags (none)
                            bs << u32_be(1); //count
                            box(u32_be('url '), [&]() {
                                bs << u32_be(0x00000001); //version (0) and flags (1)
                            }); // url
                        }); // dref
                    }); // dinf
                    box(u32_be('stbl'), [&]() {
                        box(u32_be('stsd'), [&]() {
                            bs << u32(0); //version and flags (none)
                            bs << u32_be(1); //count
                            box(u32_be('mp4a'), [&]() {
                                bs << u32(0); //reserved (6 bytes)
                                bs << u16(0);
                                bs << u16_be(1); //dref index
                                bs << u16(0); //quicktime encoding version
                                bs << u16(0); //quicktime encoding revision
                                bs << u32(0); //quicktime audio encoding vendor
                                bs << u16(0); //channels (ignored)
                                bs << u16_be(16); //sample size
                                bs << u16(0); //quicktime audio compression id
                                bs << u16(0); //quicktime audio packet size
                                bs << u32_be(sampleRateHz << 16); //sample rate (fixed point)
                                box(u32_be('esds'), [&]() {
                                    bs << u32(0); //version and flags (none)
                                    bs << u8(3); //ES descriptor type
                                                          /*bs << u8(0x80);
                                                          bs << u8(0x80);
                                                          bs << u8(0x80);*/
                                    bs << u8(esDescriptor.Num());
                                    output.Serialize((LPVOID)esDescriptor.Array(), esDescriptor.Num());
                                }); // esds
                            }); // mp4a
                        }); // stsd

                        box(u32_be('stts'), [&]() {
                            bs << u32(0); //version and flags (none)
                            bs << u32_be(audioDecodeTimes.Num());
                            for (UINT i = 0; i < audioDecodeTimes.Num(); i++)
                            {
                                bs << u32_be(audioDecodeTimes[i].count);
                                bs << u32_be(audioDecodeTimes[i].val);
                            }
                        });

                        box(u32_be('stsc'), [&]() {
                            bs << u32(0); //version and flags (none)
                            bs << u32_be(audioSampleToChunk.Num());
                            for (UINT i = 0; i < audioSampleToChunk.Num(); i++)
                            {
                                SampleToChunk &stc = audioSampleToChunk[i];
                                bs << u32_be(stc.firstChunkID);
                                bs << u32_be(stc.samplesPerChunk);
                                bs << u32(u32_be(1));
                            }
                        });

                        box(u32_be('stsz'), [&]() {
                            bs << u32(0); //version and flags (none)
                            bs << u32(0); //block size for all (0 if differing sizes)
                            bs << u32_be(audioFrames.Num());
                            for (UINT i = 0; i < audioFrames.Num(); i++) {
                                bs << u32_be(audioFrames[i].size);
                            }
                        });

                        if (audioChunks.Num() && audioChunks.Last() > 0xFFFFFFFFLL)
                        {
                            box(u32_be('co64'), [&]() {
                                bs << u32(0); //version and flags (none)
                                bs << u32_be(audioChunks.Num())
                                for (UINT i = 0; i < audioChunks.Num(); i++) {
                                    bs << u64_be(audioChunks[i]);
                                }
                            });
                        }
                        else
                        {
                            box(u32_be('stco'), [&]() {
                                bs << u32(0); //version and flags (none)
                                bs << u32_be(audioChunks.Num());
                                for (UINT i = 0; i < audioChunks.Num(); i++) {
                                    bs << u32_be((DWORD)audioChunks[i]);
                                }
                            });
                        }
                    }); // stbl
                }); // minf
            }); // mdia
        }); // trak
    });



          //SendMessage(GetDlgItem(hwndProgressDialog, IDC_PROGRESS1), PBM_SETPOS, 50, 0);
          //ProcessEvents();

          //------------------------------------------------------
          // video track
          PushBox(output, u32_be('trak'));
            PushBox(output, u32_be('tkhd')); //track header
              bs << u32(u32_be(0x00000007)); //version (0) and flags (0x7)
              bs << u32(macTime); //creation time
              bs << u32(macTime); //modified time
              bs << u32(u32_be(2)); //track ID
              bs << u32(0); //reserved
              bs << u32(videoDuration); //duration (in time base units)
              bs << u64(0); //reserved
              bs << u16(0); //video layer (0)
              bs << u16(0); //quicktime alternate track id (0)
              bs << u16(0); //track audio volume (this is video, so 0)
              bs << u16(0); //reserved
              bs << u32_be(0x00010000) << u32_be(0x00000000) << u32_be(0x00000000); //window matrix row 1 (1.0, 0.0, 0.0)
              bs << u32_be(0x00000000) << u32_be(0x00010000) << u32_be(0x00000000); //window matrix row 2 (0.0, 1.0, 0.0)
              bs << u32_be(0x00000000) << u32_be(0x00000000) << u32_be(0x40000000); //window matrix row 3 (0.0, 0.0, 16384.0)
              bs << u32_be(width << 16);  //width (fixed point)
              bs << u32_be(height << 16); //height (fixed point)
            PopBox(output); //tkhd
            PushBox(output, u32_be('mdia'));
              PushBox(output, u32_be('mdhd'));
                bs << u32(0); //version and flags (none)
                bs << u32(macTime); //creation time
                bs << u32(macTime); //modified time
                bs << u32_be(1000); //time scale
                bs << u32(videoDuration);
                bs << u32_be(0x55c40000);
              PopBox(output); //mdhd
              PushBox(output, u32_be('hdlr'));
                bs << u32(0); //version and flags (none)
                bs << u32(0); //quicktime type (none)
                bs << u32_be('vide'); //media type
                bs << u32(0); //manufacturer reserved
                bs << u32(0); //quicktime component reserved flags
                bs << u32(0); //quicktime component reserved mask
                output.Serialize((LPVOID)lpVideoTrack, (DWORD)strlen(lpVideoTrack)+1); //track name
              PopBox(output); //hdlr
              PushBox(output, u32_be('minf'));
                PushBox(output, u32_be('vmhd'));
                  bs << u32_be(0x00000001); //version (0) and flags (1)
                  bs << u16(0); //quickdraw graphic mode (copy = 0)
                  bs << u16(0); //quickdraw red value
                  bs << u16(0); //quickdraw green value
                  bs << u16(0); //quickdraw blue value
                PopBox(output); //vdhd
                PushBox(output, u32_be('dinf'));
                  PushBox(output, u32_be('dref'));
                    bs << u32(0); //version and flags (none)
                    bs << u32_be(1); //count
                    PushBox(output, u32_be('url '));
                      bs << u32_be(0x00000001); //version (0) and flags (1)
                    PopBox(output); //url
                  PopBox(output); //dref
                PopBox(output); //dinf
                PushBox(output, u32_be('stbl'));
                  PushBox(output, u32_be('stsd'));
                    bs << u32(0); //version and flags (none)
                    bs << u32_be(1); //count
                    PushBox(output, u32_be('avc1'));
                      bs << u32(0); //reserved 6 bytes
                      bs << u16(0);
                      bs << u16_be(1); //index
                      bs << u16(0); //encoding version
                      bs << u16(0); //encoding revision level
                      bs << u32(0); //encoding vendor
                      bs << u32(0); //temporal quality
                      bs << u32(0); //spatial quality
                      bs << u16_be(width); //width
                      bs << u16_be(height); //height
                      bs << u32_be(0x00480000); //fixed point width pixel resolution (72.0)
                      bs << u32_be(0x00480000); //fixed point height pixel resolution (72.0)
                      bs << u32(0); //quicktime video data size 
                      bs << u16_be(1); //frame count(?)
                      bs << u8(strlen(videoCompressionName)); //compression name length
                      output.Serialize(videoCompressionName, 31); //31 bytes for the name
                      bs << u16(u16_be(24)); //bit depth
                      bs << u16(0xFFFF); //quicktime video color table id (none = -1)
                      PushBox(output, u32_be('avcC'));
                        bs << u8(1); //version
                        bs << u8(100); //h264 profile ID
                        bs << u8(0); //h264 compatible profiles
                        bs << u8(0x1f); //h264 level
                        bs << u8(0xff); //reserved
                        bs << u8(0xe1); //first half-byte = no clue. second half = sps count
                        bs << u16_be(SPS.Num()); //sps size
                        output.Serialize(SPS.Array(), SPS.Num()); //sps data
                        bs << u8(1); //pps count
                        bs << u16_be(PPS.Num()); //pps size
                        output.Serialize(PPS.Array(), PPS.Num()); //pps data
                      PopBox(output); //avcC
                    PopBox(output); //avc1
                  PopBox(output); //stsd
                  PushBox(output, u32_be('stts')); //frame times
                    bs << u32(0); //version and flags (none)
                    bs << u32_be(videoDecodeTimes.Num());
                    for(UINT i=0; i<videoDecodeTimes.Num(); i++)
                    {
                        bs << u32_be(videoDecodeTimes[i].count);
                        bs << u32_be(videoDecodeTimes[i].val);
                    }
                  PopBox(output); //stts

                  //SendMessage(GetDlgItem(hwndProgressDialog, IDC_PROGRESS1), PBM_SETPOS, 60, 0);
                  //ProcessEvents();

                  if (IFrameIDs.Num())
                  {
                      PushBox(output, u32_be('stss')); //list of keyframe (i-frame) IDs
                        bs << u32(0); //version and flags (none)
                        bs << u32_be(IFrameIDs.Num());
                        output.Serialize(IFrameIDs.Array(), IFrameIDs.Num()*sizeof(UINT));
                      PopBox(output); //stss
                  }
                  PushBox(output, u32_be('ctts')); //list of composition time offsets
                    bs << u32(0); //version (0) and flags (none)
                    //bs << u32(u32_be(0x01000000)); //version (1) and flags (none)

                    bs << u32_be(compositionOffsets.Num()));
                    for(UINT i=0; i<compositionOffsets.Num(); i++)
                    {
                        bs << u32_be(compositionOffsets[i].count);
                        bs << u32_be(compositionOffsets[i].val);
                    }
                  PopBox(output); //ctts

                  //SendMessage(GetDlgItem(hwndProgressDialog, IDC_PROGRESS1), PBM_SETPOS, 70, 0);
                  //ProcessEvents();

                  PushBox(output, u32_be('stsc')); //sample to chunk list
                    bs << u32(0); //version and flags (none)
                    bs << u32_be(videoSampleToChunk.Num());
                    for(UINT i=0; i<videoSampleToChunk.Num(); i++)
                    {
                        SampleToChunk &stc  = videoSampleToChunk[i];
                        bs << u32_be(stc.firstChunkID);
                        bs << u32_be(stc.samplesPerChunk);
                        bs << u32_be(1);
                    }
                  PopBox(output); //stsc
                  PushBox(output, u32_be('stsz')); //sample sizes
                    bs << u32(0); //version and flags (none)
                    bs << u32(0); //block size for all (0 if differing sizes)
                    bs << u32_be(videoFrames.Num());
                    for(UINT i=0; i<videoFrames.Num(); i++)
                        bs << u32_be(videoFrames[i].size);
                  PopBox(output);

                  if(videoChunks.Num() && videoChunks.Last() > 0xFFFFFFFFLL)
                  {
                      PushBox(output, u32_be('co64')); //chunk offsets
                      bs << u32(0); //version and flags (none)
                      bs << u32_be(videoChunks.Num());
                      for(UINT i=0; i<videoChunks.Num(); i++)
                          bs << u64_be(videoChunks[i]);
                      PopBox(output); //co64
                  }
                  else
                  {
                      PushBox(output, u32_be('stco')); //chunk offsets
                        bs << u32(0); //version and flags (none)
                        bs << u32_be(videoChunks.Num());
                        for(UINT i=0; i<videoChunks.Num(); i++)
                            bs << u32_be((DWORD)videoChunks[i]);
                      PopBox(output); //stco
                  }
                PopBox(output); //stbl
              PopBox(output); //minf
            PopBox(output); //mdia
          PopBox(output); //trak

          //SendMessage(GetDlgItem(hwndProgressDialog, IDC_PROGRESS1), PBM_SETPOS, 80, 0);
          //ProcessEvents();

          //------------------------------------------------------
          // info thingy
          PushBox(output, u32_be('udta'));
            PushBox(output, u32_be('meta'));
              bs << u32(0); //version and flags (none)
              PushBox(output, u32_be('hdlr'));
                bs << u32(0); //version and flags (none)
                bs << u32(0); //quicktime type
                bs << u32_be('mdir'); //metadata type
                bs << u32_be('appl'); //quicktime manufacturer reserved thingy
                bs << u32(0); //quicktime component reserved flag
                bs << u32(0); //quicktime component reserved flag mask
                bs << u8(0); //null string
              PopBox(output); //hdlr
              PushBox(output, u32_be('ilst'));
                PushBox(output, u32_be('\xa9too'));
                  PushBox(output, u32_be('data'));
                    bs << u32_be(1); //version (1) + flags (0)
                    bs << u32(0); //reserved
                    LPSTR lpVersion = OBS_VERSION_STRING_ANSI;
                    output.Serialize(lpVersion, (DWORD)strlen(lpVersion));
                  PopBox(output); //data
                PopBox(output); //@too
              PopBox(output); //ilst
            PopBox(output); //meta
          PopBox(output); //udta

        PopBox(output); //moov
//
//        fileOut.Serialize(endBuffer.Array(), (DWORD)output.GetPos());
//        fileOut.Close();
//
//        XFile file;
//        if(file.Open(strFile, XFILE_WRITE, XFILE_OPENEXISTING))
//        {
//#ifdef USE_64BIT_MP4
//            file.SetPos((INT64)mdatStart+8, XFILE_BEGIN);
//
//            UINT64 size = fastHtonll(mdatStop-mdatStart);
//            file.Write(&size, 8);
//#else
//            file.SetPos((INT64)mdatStart, XFILE_BEGIN);
//            UINT size = fastHtonl((DWORD)(mdatStop-mdatStart));
//            file.Write(&size, 4);
//#endif
//            file.Close();
//        }

#endif
}
