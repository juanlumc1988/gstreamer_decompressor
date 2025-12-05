#include "decompressor.hpp"

#include <zlib.h>
#include <cstring>
#include <stdexcept>

/*
 * GzipDecompressor: one-shot decompression for gzip streams using zlib.
 */
class GzipDecompressor : public Decompressor {
public:
    bool decompress(const std::vector<std::uint8_t>& input,
                    std::vector<std::uint8_t>& output) override
    {
        if (input.empty()) {
            output.clear();
            return true;
        }

        // Initial guess for output size. We will grow the buffer if needed.
        std::size_t out_capacity = input.size() * 4 + 1024;
        if (out_capacity == 0) {
            out_capacity = 1024;
        }

        output.clear();
        output.resize(out_capacity);

        z_stream strm{};
        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = Z_NULL;
        strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.data()));
        strm.avail_in = static_cast<uInt>(input.size());

        // 16 + MAX_WBITS enables gzip decoding with automatic header detection.
        int ret = inflateInit2(&strm, 16 + MAX_WBITS);
        if (ret != Z_OK) {
            output.clear();
            return false;
        }

        bool ok = false;

        for (int attempt = 0; attempt < 2 && !ok; ++attempt) {
            strm.next_out  = reinterpret_cast<Bytef*>(output.data());
            strm.avail_out = static_cast<uInt>(output.size());

            ret = inflate(&strm, Z_FINISH);

            if (ret == Z_STREAM_END) {
                // Decompression completed.
                std::size_t produced = output.size() - strm.avail_out;
                output.resize(produced);
                ok = true;
            } else if (ret == Z_BUF_ERROR || ret == Z_OK) {
                // Output buffer too small; try to grow and retry once from scratch.
                output.resize(output.size() * 2);
                // Reset stream to initial state and try again.
                inflateEnd(&strm);
                strm.zalloc = Z_NULL;
                strm.zfree  = Z_NULL;
                strm.opaque = Z_NULL;
                strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.data()));
                strm.avail_in = static_cast<uInt>(input.size());
                ret = inflateInit2(&strm, 16 + MAX_WBITS);
                if (ret != Z_OK) {
                    output.clear();
                    return false;
                }
            } else {
                // Any other error is fatal.
                output.clear();
                inflateEnd(&strm);
                return false;
            }
        }

        inflateEnd(&strm);
        if (!ok) {
            output.clear();
        }

        return ok;
    }
};
