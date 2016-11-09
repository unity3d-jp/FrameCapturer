#include "pch.h"
#include "fcMP4Internal.h"
#include "fcH264Encoder.h"

void fcH264Frame::gatherNALInformation()
{
    // gather NAL information
    static const char start_seq[] = { 0, 0, 1 }; // NAL start sequence
    char *beg = data.data();
    char *end = beg + data.size();
    for (;;) {
        auto *pos = std::search(beg, end, start_seq, start_seq + 3);
        if (pos == end) { break; }
        auto *next = std::search(pos + 1, end, start_seq, start_seq + 3);
        nal_sizes.push_back(int(next - pos));
        beg = next;
    }
}
