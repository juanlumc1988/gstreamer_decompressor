#include "decompressor.hpp"

#include <zstd.h>
#include <cstring>  // for std::memcpy

/*
 * ZstdDecompressor: one-shot decompressor for Zstandard data.
 */
class ZstdDecompressor : public Decompressor {
public:
    bool decompress(const std::vector<std::uint8_t>& input,
                    std::vector<std::uint8_t>& output) override
    {
        if (input.empty()) {
            output.clear();
            return true;
        }

        /* Ask zstd for the expected decompressed size if available. */
        unsigned long long const content_size =
            ZSTD_getFrameContentSize(input.data(), input.size());

        size_t out_capacity = 0;

        if (content_size != ZSTD_CONTENTSIZE_ERROR &&
            content_size != ZSTD_CONTENTSIZE_UNKNOWN) {
            out_capacity = static_cast<size_t>(content_size);
        } else {
            /* Fallback heuristic when size is unknown:
             * This is a simple trade-off between memory usage and
             * not failing for moderately compressed streams.
             */
            out_capacity = input.size() * 4 + 1024;
        }

        if (out_capacity == 0) {
            output.clear();
            return true;
        }

        output.resize(out_capacity);

        size_t const decompressed_size = ZSTD_decompress(
            output.data(), out_capacity,
            input.data(), input.size());

        if (ZSTD_isError(decompressed_size)) {
            output.clear();
            return false;
        }

        if (decompressed_size > out_capacity) {
            // Should not happen, but guard against it.
            output.clear();
            return false;
        }

        output.resize(decompressed_size);
        return true;
    }
};
