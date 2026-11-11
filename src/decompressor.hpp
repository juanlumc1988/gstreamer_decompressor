#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <vector>

/// Interface for all decompressor implementations.
///
/// This class defines a simple one-shot decompression contract:
///  - The whole compressed payload is provided as a single input buffer.
///  - The decompressed payload is returned via the output vector.
///  - The method returns true on success, false on failure.
///
/// The size of the decompressed payload is not known in advance and is not
/// estimated: implementations decompress incrementally into an output vector
/// that grows as needed (limited only by available memory).
class Decompressor {
public:
    virtual ~Decompressor() = default;

    /// Decompresses the given input buffer into the output vector.
    ///
    /// Concatenated streams (several gzip members, bzip2 streams or zstd
    /// frames back to back) are decompressed and appended in order. Truncated
    /// input, corrupt data and trailing bytes that are not a valid stream are
    /// errors.
    ///
    /// @param input  Entire compressed payload.
    /// @param output Output vector that will be filled with decompressed bytes.
    ///               The vector is cleared and resized as needed.
    /// @return true  If decompression succeeds.
    /// @return false If the compressed data is invalid or any error occurs
    ///               (including running out of memory). The output is cleared.
    virtual bool decompress(const std::vector<std::uint8_t>& input, std::vector<std::uint8_t>& output) = 0;
};

namespace decompressor_detail {

/// Smallest output capacity used when an output vector first has to grow.
constexpr std::size_t kMinOutputCapacity = 64 * 1024;

/// Doubles the size of @p out (to at least kMinOutputCapacity).
///
/// Reports failure instead of throwing when the size would overflow or the
/// allocation fails, so that no exception ever crosses a decompress() call.
inline bool grow(std::vector<std::uint8_t>& out)
{
    const std::size_t current = out.size();
    if (current > std::numeric_limits<std::size_t>::max() / 2) {
        return false;
    }
    try {
        out.resize(std::max(current * 2, kMinOutputCapacity));
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

}  // namespace decompressor_detail
