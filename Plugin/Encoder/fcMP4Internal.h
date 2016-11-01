#pragma once

#include "fcFoundation.h"



struct fcMP4FrameInfo
{
    size_t size;
    uint64_t file_offset;
    fcTime timestamp;

    fcMP4FrameInfo()
        : size(), file_offset(), timestamp()
    {}
};

struct fcMP4OffsetValue
{
    uint32_t count;
    uint32_t value;
};

struct fcMP4SampleToChunk
{
    uint32_t first_chunk_ID;
    uint32_t samples_per_chunk;
    uint32_t sample_description_ID;
};

typedef std::function<void(fcDownloadState, const char *message)> fcDownloadCallback;

const std::string& fcMP4GetModulePath();
const std::string& fcMP4GetFAACPackagePath();
