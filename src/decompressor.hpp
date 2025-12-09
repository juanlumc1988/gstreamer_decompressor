#pragma once

#include <cstdint>
#include <memory>
#include <vector>

/// Interface for all decompressor implementations.
///
/// This class defines a simple one-shot decompression contract:
///  - The whole compressed payload is provided as a single input buffer.
///  - The decompressed payload is returned via the output vector.
///  - The method returns true on success, false on failure.
class Decompressor {
public:
    virtual ~Decompressor() = default;

    /// Decompresses the given input buffer into the output vector.
    ///
    /// @param input  Entire compressed payload.
    /// @param output Output vector that will be filled with decompressed bytes.
    ///               The vector is cleared and resized as needed.
    /// @return true  If decompression succeeds.
    /// @return false If the compressed data is invalid or any error occurs.
    virtual bool decompress(const std::vector<std::uint8_t>& input, std::vector<std::uint8_t>& output) = 0;
};
