#pragma once

#include <cstdint>
#include <memory>
#include <vector>

/*
 * Abstract interface for a generic decompressor.
 * Concrete implementations will handle specific formats
 * such as zstd, gzip or bzip2.
 */
class Decompressor {
public:
    virtual ~Decompressor() = default;

    /*
     * Decompresses the given input buffer into the provided output vector.
     * Returns true on success, false on failure.
     *
     * Implementations are free to resize the output vector as needed.
     */
    virtual bool decompress(const std::vector<std::uint8_t>& input,
                            std::vector<std::uint8_t>& output) = 0;
};
