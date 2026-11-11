#pragma once

#include "decompressor.hpp"

#include <bzlib.h>
#include <climits>

/// @brief bzip2 decompressor implementation using libbz2.
///
/// This class uses libbz2's streaming API (BZ2_bzDecompress()) over the
/// in-memory input. The output size is never estimated, so highly
/// compressible data works as well as any other.
///
/// Strategy:
///  - Call BZ2_bzDecompress() into the output vector, doubling it whenever it
///    is full.
///  - After BZ_STREAM_END, if input remains, start a new stream and continue
///    (concatenated bzip2 streams, like bunzip2).
///  - Treat truncated input, corrupt data, trailing bytes that are not a
///    bzip2 stream and any call that makes no progress as errors.
///
/// Empty input is treated as a successful no-op and produces an empty output.
class Bzip2Decompressor : public Decompressor {
public:
    /// @copydoc Decompressor::decompress()
    bool decompress(const std::vector<std::uint8_t>& input, std::vector<std::uint8_t>& output) override
    {
        output.clear();
        if (input.empty()) {
            return true;
        }

        bz_stream strm{};
        if (BZ2_bzDecompressInit(&strm, 0, 0) != BZ_OK) {
            return false;
        }

        std::size_t in_pos = 0;   // bytes handed to libbz2 so far
        std::size_t out_pos = 0;  // bytes produced so far
        bool ok = false;

        for (;;) {
            if (strm.avail_in == 0 && in_pos < input.size()) {
                const std::size_t chunk = std::min<std::size_t>(input.size() - in_pos, UINT_MAX);
                strm.next_in = const_cast<char*>(reinterpret_cast<const char*>(input.data() + in_pos));
                strm.avail_in = static_cast<unsigned int>(chunk);
                in_pos += chunk;
            }
            if (out_pos == output.size() && !decompressor_detail::grow(output)) {
                break;
            }

            const std::size_t out_room = std::min<std::size_t>(output.size() - out_pos, UINT_MAX);
            const unsigned int in_before = strm.avail_in;
            strm.next_out = reinterpret_cast<char*>(output.data() + out_pos);
            strm.avail_out = static_cast<unsigned int>(out_room);

            const int ret = BZ2_bzDecompress(&strm);
            const std::size_t produced = out_room - strm.avail_out;
            out_pos += produced;

            if (ret == BZ_STREAM_END) {
                if (strm.avail_in == 0 && in_pos == input.size()) {
                    ok = true;
                    break;
                }
                // More input follows: restart libbz2 for the next stream
                // without losing the unread input.
                char* const rest = strm.next_in;
                const unsigned int rest_len = strm.avail_in;
                BZ2_bzDecompressEnd(&strm);
                strm = bz_stream{};
                if (BZ2_bzDecompressInit(&strm, 0, 0) != BZ_OK) {
                    // strm is not initialised: do not call ...End() on it.
                    output.clear();
                    return false;
                }
                strm.next_in = rest;
                strm.avail_in = rest_len;
                continue;
            }
            // Every other error code is a failure; BZ_OK must have made
            // progress (otherwise the input is truncated).
            if (ret != BZ_OK || (produced == 0 && strm.avail_in == in_before)) {
                break;
            }
        }

        BZ2_bzDecompressEnd(&strm);
        if (!ok) {
            output.clear();
            return false;
        }
        output.resize(out_pos);
        return true;
    }
};
