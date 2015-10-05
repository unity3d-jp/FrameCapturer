#include "pch.h"
#include <fstream>
#include "lsmash/lsmash.h"
#include "lsmash/importer.h"
#include "fcFoundation.h"
#include "fcMP4Muxer.h"


#ifdef fcWindows
    #pragma comment(lib, "liblsmash.lib")
#else 
#endif





fcMP4Muxer::fcMP4Muxer()
{
}

fcMP4Muxer::~fcMP4Muxer()
{
}


struct MP4TrackData
{
    importer_t          *importer;
    lsmash_summary_t    *summary;
    lsmash_sample_t     *sample;
    int                 sample_entry;
    int                 track_number;
    int                 track_id;
    uint32_t            timescale;
    uint32_t            timebase;
    double              dts;
    double              prev_dts;

    MP4TrackData() { memset(this, 0, sizeof(*this)); }
};

bool fcMP4Muxer::mux(const Params &params)
{
    lsmash_root_t               *root;
    lsmash_file_parameters_t    mp4_stream;
    lsmash_file_parameters_t    h264_stream;
    lsmash_brand_type           major_brand = ISOM_BRAND_TYPE_MP42;
    lsmash_brand_type           compatible_brands[2] = { ISOM_BRAND_TYPE_MP42, ISOM_BRAND_TYPE_ISOM };
    uint32_t                    fps_num = params.frame_rate;
    uint32_t                    fps_den = 1;

    // 出力 mp4
    root = lsmash_create_root();
    if (lsmash_open_file(params.out_mp4_path, 0, &mp4_stream) != 0) {
        return false;
    }
    mp4_stream.major_brand = major_brand;
    mp4_stream.brands = compatible_brands;
    mp4_stream.brand_count = sizeof(compatible_brands) / sizeof(compatible_brands[0]);
    mp4_stream.minor_version = 0;
    lsmash_set_file(root, &mp4_stream);

    lsmash_movie_parameters_t movie_param;
    lsmash_initialize_movie_parameters(&movie_param);
    lsmash_set_movie_parameters(root, &movie_param);


    int track_number = 1;
    MP4TrackData track_data[2];
    int num_track_data = 0;

    if (params.in_h264_path) {
        importer_t *h264_importer = lsmash_importer_open(params.in_h264_path, "H.264");
        if (h264_importer == nullptr) { return false; }

        int h264_track_id = lsmash_create_track(root, ISOM_MEDIA_HANDLER_TYPE_VIDEO_TRACK);

        lsmash_track_parameters_t track_param;
        lsmash_initialize_track_parameters(&track_param);
        (int&)track_param.mode = ISOM_TRACK_IN_MOVIE | ISOM_TRACK_IN_PREVIEW;

        int sample_entry = 0;
        lsmash_summary_t *summary = lsmash_duplicate_summary(h264_importer, track_number);
        auto video_summary = (lsmash_video_summary_t*)summary;
        track_param.display_width = video_summary->width << 16;
        track_param.display_height = video_summary->height << 16;
        lsmash_set_track_parameters(root, h264_track_id, &track_param);
        sample_entry = lsmash_add_sample_entry(root, h264_track_id, summary);

        auto& td = track_data[num_track_data++];
        td.importer = h264_importer;
        td.summary = summary;
        td.sample_entry = sample_entry;
        td.track_number = track_number;
        td.track_id = h264_track_id;
        td.timescale = video_summary->timescale;
        td.timebase = video_summary->timebase;

        lsmash_media_parameters_t media_param;
        lsmash_initialize_media_parameters(&media_param);
        media_param.timescale = td.timescale;
        lsmash_set_media_parameters(root, h264_track_id, &media_param);
    }

    if (params.in_aac_path) {
        importer_t *aac_importer = lsmash_importer_open(params.in_aac_path, "adts");
        if (aac_importer == nullptr) { return false; }

        int aac_track_id = lsmash_create_track(root, ISOM_MEDIA_HANDLER_TYPE_AUDIO_TRACK);

        lsmash_track_parameters_t track_param;
        lsmash_initialize_track_parameters(&track_param);
        (int&)track_param.mode = ISOM_TRACK_IN_MOVIE | ISOM_TRACK_IN_PREVIEW;

        int sample_entry = 0;
        lsmash_summary_t *summary = lsmash_duplicate_summary(aac_importer, track_number);
        auto audio_summary = (lsmash_audio_summary_t*)summary;
        lsmash_set_track_parameters(root, aac_track_id, &track_param);
        sample_entry = lsmash_add_sample_entry(root, aac_track_id, summary);

        auto& td = track_data[num_track_data++];
        td.importer = aac_importer;
        td.summary = summary;
        td.sample_entry = sample_entry;
        td.track_number = track_number;
        td.track_id = aac_track_id;
        td.timescale = audio_summary->frequency;
        td.timebase = 1;

        lsmash_media_parameters_t media_param;
        lsmash_initialize_media_parameters(&media_param);
        media_param.timescale = td.timescale;
        lsmash_set_media_parameters(root, aac_track_id, &media_param);
    }

    double largest_dts = 0.0;
    uint32_t num_consecutive_sample_skip = 0;
    for (int ti = 0;;) {
        auto &td = track_data[ti];

        if (!td.sample) {
            int ret = lsmash_importer_get_access_unit(td.importer, td.track_number, &td.sample);
            if (ret <= -1) // error
            {
                lsmash_delete_sample(td.sample);
                break;
            }
            else if (ret == 1) /* a change of stream's properties */
            {
                lsmash_cleanup_summary(td.summary);
                td.summary = lsmash_duplicate_summary(td.importer, td.track_number);
                td.sample_entry = lsmash_add_sample_entry(root, td.track_id, td.summary);
                if (!td.sample_entry) { break; }
            }
            else if (ret == 2) /* EOF */
            {
                lsmash_delete_sample(td.sample);
                break;
            }

            if (td.sample)
            {
                td.sample->index = td.sample_entry;
                td.sample->dts *= td.timebase;
                td.sample->cts *= td.timebase;
                td.dts = (double)td.sample->dts / td.timescale;
            }
        }

        if (td.sample) {
            if (td.dts <= largest_dts || num_consecutive_sample_skip == num_track_data)
            {
                uint64_t sample_size = td.sample->length;
                uint64_t sample_dts = td.sample->dts;
                uint64_t sample_cts = td.sample->cts;
                if (lsmash_append_sample(root, td.track_id, td.sample)) {
                    return false;
                }
                td.prev_dts = sample_dts;
                td.sample = nullptr;
                largest_dts = std::max<double>(largest_dts, td.dts);
                num_consecutive_sample_skip = 0;
            }
            else {
                // skip
                ++num_consecutive_sample_skip;
            }
        }

        ti = (ti + 1) % num_track_data;
    }

    for (int i = 0; i < num_track_data; ++i) {
        lsmash_flush_pooled_samples(root, track_data[i].track_id, fps_den);
        lsmash_cleanup_summary(track_data[i].summary);
        lsmash_importer_close(track_data[i].importer);
    }


    lsmash_finish_movie(root, nullptr);
    lsmash_close_file(&mp4_stream);
    lsmash_destroy_root(root);

    return true;
}

//bool fcMP4Muxer::mux(std::ostream *out, std::istream *in_h264); // todo
