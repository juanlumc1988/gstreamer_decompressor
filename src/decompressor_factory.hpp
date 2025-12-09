#pragma once

#include "decompressor.hpp"

/// @brief Factory for Decompressor implementations.
///
/// This class inspects the first bytes (magic bytes) of the compressed input
/// and returns an appropriate Decompressor strategy:
///  - gzip   if the input starts with 0x1F 0x8B
///  - bzip2  if the input starts with 'B' 'Z' 'h'
///  - zstd   otherwise (default)
///
/// The goal is to keep format detection logic centralized, so that the
/// GStreamer element does not need to know about gzip/bzip2 details.
class DecompressorFactory {
public:
    /// @brief Creates a decompressor instance based on the input magic bytes.
    ///
    /// If the input is too small to contain a full magic sequence, the missing
    /// bytes are treated as zero and detection falls back to the default zstd
    /// implementation.
    ///
    /// @param input Compressed data buffer whose first bytes are used as magic.
    /// @return A unique_ptr to a concrete Decompressor implementation.
    static std::unique_ptr<Decompressor> create(const std::vector<std::uint8_t>& input);
};
