/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#define MP4V2_USE_STATIC_LIB
#include "mp4v2/mp4v2.h"
#include "openh264/codec_api.h"
#include <cstdint>
#include <cstdlib>
#include <windows.h>

#pragma comment(lib, "libmp4v2.lib")

#if defined(_M_AMD64)
    #define OpenH264DLL "openh264-1.4.0-win64msvc.dll"
#elif defined(_M_IX86)
    #define OpenH264DLL "openh264-1.4.0-win32msvc.dll"
#endif

typedef int  (*WelsCreateSVCEncoderT)(ISVCEncoder** ppEncoder);


#define OutputFileName "test.mp4"


int main(int argc, char** argv)
{
    HMODULE h264_dll = nullptr;
    ISVCEncoder *h264_encoder = nullptr;
    {
        h264_dll = ::LoadLibraryA(OpenH264DLL);
        if (h264_dll != nullptr) {
            WelsCreateSVCEncoderT pWelsCreateSVCEncoder = (WelsCreateSVCEncoderT)::GetProcAddress(h264_dll, "WelsCreateSVCEncoder");
            if (pWelsCreateSVCEncoder != nullptr)
            {
                pWelsCreateSVCEncoder(&h264_encoder);
            }
        }
    }

    uint32_t verbosity = 0;

    MP4FileHandle mp4File = MP4Create(OutputFileName, verbosity);

    if (!mp4File) {
        exit(1);
    }

    printf("Created skeleton\n");
    MP4Dump(mp4File);

    MP4SetODProfileLevel(mp4File, 1);
    MP4SetSceneProfileLevel(mp4File, 1);
    MP4SetVideoProfileLevel(mp4File, 1);
    MP4SetAudioProfileLevel(mp4File, 1);
    MP4SetGraphicsProfileLevel(mp4File, 1);

    MP4TrackId odTrackId = 
        MP4AddODTrack(mp4File);

    MP4TrackId bifsTrackId = 
        MP4AddSceneTrack(mp4File);

    MP4TrackId videoTrackId = MP4AddH264VideoTrack(mp4File, 90000, 3000, 320, 240, 1, 2, 3, 1);
    static uint8_t pseq[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    MP4AddH264SequenceParameterSet(mp4File, videoTrackId, pseq, 10);
    MP4AddH264SequenceParameterSet(mp4File, videoTrackId, pseq, 6);
    MP4AddH264PictureParameterSet(mp4File, videoTrackId, pseq, 7);
    MP4AddH264PictureParameterSet(mp4File, videoTrackId, pseq, 8);
    MP4AddH264PictureParameterSet(mp4File, videoTrackId, pseq, 7);

    MP4TrackId videoHintTrackId = 
        MP4AddHintTrack(mp4File, videoTrackId);

    MP4TrackId audioTrackId = 
        MP4AddAudioTrack(mp4File, 44100, 1152);

    MP4TrackId audioHintTrackId = 
        MP4AddHintTrack(mp4File, audioTrackId);

    printf("Added tracks\n");
    MP4Dump(mp4File);

    MP4Close(mp4File);

    //	MP4MakeIsmaCompliant(argv[1], verbosity);

    exit(0);
}

