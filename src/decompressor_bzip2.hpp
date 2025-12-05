#include "decompressor.hpp"

#include <bzlib.h>
#include <cstring>

/*
 * Bzip2Decompressor: one-shot decompression for bzip2 streams.
 */
class Bzip2Decompressor : public Decompressor {
public:
    bool decompress(const std::vector<std::uint8_t>& input,
                    std::vector<std::uint8_t>& output) override
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
