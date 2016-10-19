#include "pch.h"
#include "fcFoundation.h"
#include "fcI420.h"
#include "fcVorbisEncoder.h"
#include "fcVPXEncoder.h"
#include "fcWebMMuxer.h"

#include "webm/mkvparser.hpp"
#include "webm/mkvreader.hpp"
#include "webm/mkvwriter.hpp"
#include "webm/mkvmuxer.hpp"
#include "webm/mkvmuxerutil.hpp"


fcWebMMuxer::fcWebMMuxer(BinaryStream &stream, const fcWebMConfig &conf)
    : m_stream(stream)
    , m_conf(conf)
{
}

fcWebMMuxer::~fcWebMMuxer()
{
}

void fcWebMMuxer::addVideoFrame(const fcVPXFrame& buf)
{
}

void fcWebMMuxer::addAudioFrame(const fcVorbisFrame& buf)
{
}
