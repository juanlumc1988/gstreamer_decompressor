#pragma once

#include "decompressor.hpp"

/*
 * Factory responsible for choosing the appropriate Decompressor
 * implementation based on the input data (magic bytes).
 */
class DecompressorFactory {
public:
    /*
     * Examines the input buffer, inspects magic bytes and returns
     * an appropriate Decompressor instance.
     *
     * Supported formats:
     *  - gzip:  0x1F 0x8B
     *  - bzip2: 'B' 'Z' 'h'
     *  - default: zstd
     */
    static std::unique_ptr<Decompressor> create(const std::vector<std::uint8_t>& input);
};
