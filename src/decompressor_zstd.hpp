#pragma once

#include "decompressor.hpp"

#include <zstd.h>

/// @brief Zstandard decompressor implementation.
///
/// This class uses libzstd's streaming API (ZSTD_decompressStream()) over the
/// in-memory input. The output size is never estimated, so it works the same
/// for frames that declare their content size and for frames that do not
/// (e.g. data compressed from a pipe), whatever the compression ratio.
///
/// Strategy:
///  - Decompress into the output vector, doubling it whenever it is full.
///  - After a frame is completely decoded and flushed, continue with the next
///    frame until all the input is consumed (concatenated frames).
///
/// Error handling strategy:
///  - If the input buffer is empty, the method succeeds and clears the output.
///  - If libzstd reports an error, the input is truncated (the last frame is
///    incomplete) or memory runs out, the method clears the output and
///    returns false.
class ZstdDecompressor : public Decompressor {
public:
    /// @copydoc Decompressor::decompress()
    bool decompress(const std::vector<std::uint8_t>& input, std::vector<std::uint8_t>& output) override
    {
        output.clear();
        if (input.empty()) {
            return true;
        }

        std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> dctx(ZSTD_createDCtx(), &ZSTD_freeDCtx);
        if (!dctx) {
            return false;
        }

        ZSTD_inBuffer in{input.data(), input.size(), 0};
        std::size_t out_pos = 0;

        for (;;) {
            if (out_pos == output.size() && !decompressor_detail::grow(output)) {
                break;
            }

            ZSTD_outBuffer out{output.data(), output.size(), out_pos};
            // 0 means: current frame completely decoded and flushed.
            const std::size_t ret = ZSTD_decompressStream(dctx.get(), &out, &in);
            if (ZSTD_isError(ret)) {
                break;
            }
            out_pos = out.pos;

            const bool input_done = (in.pos == in.size);
            if (ret == 0 && input_done) {
                output.resize(out_pos);
                return true;
            }
            // Input exhausted, frame not finished and the output has room:
            // nothing more can be produced, the input is truncated.
            if (input_done && out_pos < output.size()) {
                break;
            }
        }

        output.clear();
        return false;
    }
};
