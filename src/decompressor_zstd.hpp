#include "decompressor.hpp"

#include <zstd.h>
#include <cstring>

/// @brief Zstandard decompressor implementation.
///
/// This class uses the libzstd API to decompress a complete Zstandard frame.
/// It first tries to query the uncompressed size from the frame header using
/// ZSTD_getFrameContentSize(). If that fails (unknown or error), it falls
/// back to a heuristic based on the compressed size.
///
/// Error handling strategy:
///  - If the input buffer is empty, the method succeeds and clears the output.
///  - If libzstd reports an error, the method clears the output and returns false.
///  - On success, the output vector is resized to the exact number of bytes
///    produced by ZSTD_decompress().
class ZstdDecompressor : public Decompressor {
public:
    /// @copydoc Decompressor::decompress()
    bool decompress(const std::vector<std::uint8_t>& input, std::vector<std::uint8_t>& output) override
    {
        if (input.empty()) {
            output.clear();
            return true;
        }

        /* Ask zstd for the expected decompressed size if available. */
        unsigned long long const content_size = ZSTD_getFrameContentSize(input.data(), input.size());

        size_t out_capacity = 0;

        if (content_size != ZSTD_CONTENTSIZE_ERROR && content_size != ZSTD_CONTENTSIZE_UNKNOWN) {
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

        size_t const decompressed_size = ZSTD_decompress(output.data(), out_capacity, input.data(), input.size());
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
