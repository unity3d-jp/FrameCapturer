#include "pch.h"
#include "Misc.h"

#ifdef fcWindows
    #define ZIP_STATIC
    #pragma comment(lib, "libbz2.lib")
    #pragma comment(lib, "libzip.lib")
    #pragma comment(lib, "zlibstatic.lib")
#endif
#include <bzip2/bzlib.h>
#include <libzip/zip.h>


bool BZ2Decompress(Buffer &dst, const void *src, size_t src_len)
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
    Buffer dst;
    if (!BZ2Decompress(dst, src, src_len)) {
        return 0;
    }

    FILE *fout = fopen(dst_path, "wb");
    if (fout == nullptr) { return false; }
    fwrite(&dst[0], 1, dst.size(), fout);
    fclose(fout);
    return dst.size();
}


size_t Unzip(const char *dst_path_, const char *archive, const UnzipFileHandler& handler)
{
    struct zip *za;
    struct zip_file *zf;
    struct zip_stat sb;
    char buf[1024];
    size_t ret = 0;
    std::string dst_path = dst_path_;

    int err;
    if ((za = zip_open(archive, 0, &err)) == nullptr) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        return false;
    }

    for (int i = 0; i < zip_get_num_entries(za, 0); ++i) {
        if (zip_stat_index(za, i, 0, &sb) == 0) {
            std::string dstpath = dst_path + '/' + sb.name;

            if (dstpath.back() == '/') {
                std::experimental::filesystem::create_directories(dstpath.c_str());
            }
            else {
                zf = zip_fopen_index(za, i, 0);
                if (!zf) {
                    goto bail_out;
                }

                FILE *of = fopen(dstpath.c_str(), "wb");
                if (of == nullptr) {
                    goto bail_out;
                }

                size_t sum = 0;
                while (sum != sb.size) {
                    size_t len = (size_t)zip_fread(zf, buf, sizeof(buf));
                    if (len < 0) {
                        fclose(of);
                        goto bail_out;
                    }
                    fwrite(buf, 1, len, of);
                    sum += len;
                }
                fclose(of);
                zip_fclose(zf);
                if (handler) { handler(dstpath.c_str()); }
                ++ret;
            }
        }
    }

    if (zip_close(za) == -1) {
        return false;
    }
    return ret;

bail_out:
    zip_close(za);
    return ret;
}
