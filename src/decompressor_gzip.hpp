#pragma once

#include "decompressor.hpp"

#include <zlib.h>
#include <climits>

/// @brief Gzip decompressor implementation using zlib.
///
/// This class uses zlib's inflate() API in gzip mode (inflateInit2 with
/// 16 + MAX_WBITS) over the in-memory input. The output size is never
/// estimated, so highly compressible data works as well as any other.
///
/// Strategy:
///  - Call inflate() into the output vector, doubling it whenever it is full.
///  - After Z_STREAM_END, if input remains, reset the stream and continue
///    with the next gzip member (concatenated members, like gunzip).
///  - Treat truncated input, corrupt data, trailing bytes that are not a gzip
///    member and any call that makes no progress as errors.
///
/// Empty input is treated as a successful no-op and produces an empty output.
class GzipDecompressor : public Decompressor {
public:
    /// @copydoc Decompressor::decompress()
    bool decompress(const std::vector<std::uint8_t>& input, std::vector<std::uint8_t>& output) override
    {
        output.clear();
        if (input.empty()) {
            return true;
        }

        z_stream strm{};
        if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
            return false;
        }

        std::size_t in_pos = 0;   // bytes handed to zlib so far
        std::size_t out_pos = 0;  // bytes produced so far
        bool ok = false;

        for (;;) {
            if (strm.avail_in == 0 && in_pos < input.size()) {
                const std::size_t chunk = std::min<std::size_t>(input.size() - in_pos, UINT_MAX);
                strm.next_in = const_cast<Bytef*>(input.data() + in_pos);
                strm.avail_in = static_cast<uInt>(chunk);
                in_pos += chunk;
            }
            if (out_pos == output.size() && !decompressor_detail::grow(output)) {
                break;
            }

            const std::size_t out_room = std::min<std::size_t>(output.size() - out_pos, UINT_MAX);
            const uInt in_before = strm.avail_in;
            strm.next_out = output.data() + out_pos;
            strm.avail_out = static_cast<uInt>(out_room);

            const int ret = inflate(&strm, Z_NO_FLUSH);
            const std::size_t produced = out_room - strm.avail_out;
            out_pos += produced;

            if (ret == Z_STREAM_END) {
                if (strm.avail_in == 0 && in_pos == input.size()) {
                    ok = true;
                    break;
                }
                if (inflateReset(&strm) != Z_OK) {
                    break;
                }
                continue;
            }
            // Z_BUF_ERROR (no progress possible, e.g. truncated input) and
            // every other code are failures; Z_OK must have made progress.
            if (ret != Z_OK || (produced == 0 && strm.avail_in == in_before)) {
                break;
            }
        }

        inflateEnd(&strm);
        if (!ok) {
            output.clear();
            return false;
        }
        output.resize(out_pos);
        return true;
    }
};
