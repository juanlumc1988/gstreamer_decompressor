#include "decompressor.hpp"

#include <bzlib.h>
#include <cstring>

/// @brief bzip2 decompressor implementation using libbz2.
///
/// This class wraps BZ2_bzBuffToBuffDecompress() to implement a one-shot
/// decompression API.
///
/// Strategy:
///  - Start with an output buffer sized as a heuristic multiple of the
///    compressed size.
///  - Call BZ2_bzBuffToBuffDecompress().
///  - If the result is BZ_OUTBUFF_FULL, double the buffer size and retry once.
///  - Treat any result other than BZ_OK as an error.
///
/// Empty input is treated as a successful no-op and produces an empty output.
class Bzip2Decompressor : public Decompressor {
public:
    /// @copydoc Decompressor::decompress()
    bool decompress(const std::vector<std::uint8_t>& input, std::vector<std::uint8_t>& output) override
    {
        if (input.empty()) {
            output.clear();
            return true;
        }

        // Initial guess for output size.
        unsigned int out_capacity = static_cast<unsigned int>(input.size() * 4 + 1024);
        if (out_capacity == 0) {
            out_capacity = 1024;
        }

        output.clear();
        output.resize(out_capacity);

        int bzret = BZ_OUTBUFF_FULL;

        for (int attempt = 0; attempt < 2; ++attempt) {
            unsigned int dest_len = out_capacity;
            bzret = BZ2_bzBuffToBuffDecompress(
                reinterpret_cast<char*>(output.data()),
                &dest_len,
                const_cast<char*>(reinterpret_cast<const char*>(input.data())),
                static_cast<unsigned int>(input.size()),
                0,  // small
                0   // verbosity
            );

            if (bzret == BZ_OK) {
                output.resize(dest_len);
                return true;
            }

            if (bzret == BZ_OUTBUFF_FULL) {
                // Grow buffer and try again.
                out_capacity *= 2;
                output.resize(out_capacity);
                continue;
            }

            // Any other error: fail.
            output.clear();
            return false;
        }

        // If we exhausted attempts, consider it a failure.
        output.clear();
        return false;
    }
};
