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



bool fcMP4Muxer::mux(const char *out_mp4_path, const char *in_h264_path, int frame_rate)
{
    lsmash_root_t               *root;
    lsmash_file_parameters_t    mp4_stream;
    lsmash_file_parameters_t    h264_stream;
    lsmash_brand_type           major_brand = ISOM_BRAND_TYPE_MP42;
    lsmash_brand_type           compatible_brands[2] = { ISOM_BRAND_TYPE_MP42, ISOM_BRAND_TYPE_ISOM };
    uint32_t                    fps_num = frame_rate;
    uint32_t                    fps_den = 1;

    // 入力 H264 raw data
    importer_t *importer = lsmash_importer_open(in_h264_path, "H.264");
    if (importer == nullptr) { return false; }

    // 出力 mp4
    root = lsmash_create_root();
    if (lsmash_open_file(out_mp4_path, 0, &mp4_stream)!=0) {
        lsmash_importer_close(importer);
        return false;
    }
    mp4_stream.major_brand = major_brand;
    mp4_stream.brands = compatible_brands;
    mp4_stream.brand_count = sizeof(compatible_brands) / sizeof(compatible_brands[0]);
    mp4_stream.minor_version = 0;
    lsmash_set_file(root, &mp4_stream);


    int track_id = lsmash_create_track(root, ISOM_MEDIA_HANDLER_TYPE_VIDEO_TRACK);
    lsmash_movie_parameters_t movie_param;
    lsmash_initialize_movie_parameters(&movie_param);
    lsmash_set_movie_parameters(root, &movie_param);

    lsmash_media_parameters_t media_param;
    lsmash_initialize_media_parameters(&media_param);
    media_param.timescale = fps_num;
    lsmash_set_media_parameters(root, track_id, &media_param);

    lsmash_track_parameters_t track_param;
    lsmash_initialize_track_parameters(&track_param);
    (int&)track_param.mode = ISOM_TRACK_IN_MOVIE | ISOM_TRACK_IN_PREVIEW;

    int track_number = 1;
    int sample_entry = 0;
    lsmash_summary_t *summary = nullptr;
    {
        summary = lsmash_duplicate_summary(importer, track_number);
        sample_entry = lsmash_add_sample_entry(root, track_id, summary);
        auto video_summary = (lsmash_video_summary_t*)summary;
        track_param.display_width = video_summary->width << 16;
        track_param.display_height = video_summary->height << 16;
    }
    lsmash_set_track_parameters(root, track_id, &track_param);

    for (;;) {
        lsmash_sample_t *sample = nullptr;
        int ret = lsmash_importer_get_access_unit(importer, track_number, &sample);
        if (ret <= -1) // error
        {
            lsmash_delete_sample(sample);
            break;
        }
        else if (ret == 1) /* a change of stream's properties */
        {
            lsmash_cleanup_summary(summary);
            summary = lsmash_duplicate_summary(importer, track_number);
            sample_entry = lsmash_add_sample_entry(root, track_id, summary);
            if (!sample_entry) { break; }
        }
        else if (ret == 2) /* EOF */
        {
            lsmash_delete_sample(sample);
            break;
        }

        if (sample)
        {
            sample->index = sample_entry;
            lsmash_append_sample(root, track_id, sample);
        }
    }
    lsmash_flush_pooled_samples(root, track_id, fps_den);
    lsmash_finish_movie(root, nullptr);
    lsmash_cleanup_summary(summary);
    lsmash_importer_close(importer);
    lsmash_close_file(&mp4_stream);
    lsmash_destroy_root(root);

    return true;
}

//bool fcMP4Muxer::mux(std::ostream *out, std::istream *in_h264); // todo
