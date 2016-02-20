#include "pch.h"
#include <bzip2/bzlib.h>
#include "Misc.h"

#ifdef fcWindows
    #pragma comment(lib, "libbz2.lib")
#endif

bool BZ2Decompress(std::vector<char> &dst, const void *src, size_t src_len)
{
    const size_t initial_buffer_size = 1024 * 1024;
    if (dst.empty()) { dst.resize(initial_buffer_size); }

    int ret = 0;
    for (;;) {
        uint32_t dst_len = (uint32_t)dst.size();
        BZ2_bzBuffToBuffDecompress(&dst[0], &dst_len, (char*)src, (uint32_t)src_len, 0, 0);
        if (ret == BZ_OUTBUFF_FULL) {
            dst.resize(dst.size() * 2);
        }
        else {
            dst.resize(dst_len);
            break;
        }
    }
    return ret == BZ_OK;
}

size_t BZ2DecompressToFile(const char *dst_path, const void *src, size_t src_len)
{
    std::vector<char> dst;
    if (!BZ2Decompress(dst, src, src_len)) {
        return 0;
    }

    FILE *fout = fopen(dst_path, "wb");
    if (fout == nullptr) { return false; }
    fwrite(&dst[0], 1, dst.size(), fout);
    fclose(fout);
    return dst.size();
}
