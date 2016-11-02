#pragma once

#include "fcFoundation.h"



struct fcMP4FrameInfo
{
    size_t size = 0;
    uint64_t file_offset = 0;
    uint64_t timestamp = 0;
};

struct fcMP4OffsetValue
{
    uint32_t count = 0;
    uint32_t value = 0;
};

struct fcMP4SampleToChunk
{
    uint32_t first_chunk_ID = 0;
    uint32_t samples_per_chunk = 0;
    uint32_t sample_description_ID = 0;
};

typedef std::function<void(fcDownloadState, const char *message)> fcDownloadCallback;

const std::string& fcMP4GetModulePath();
const std::string& fcMP4GetFAACPackagePath();
