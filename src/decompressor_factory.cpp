#include "decompressor_factory.hpp"
#include "decompressor.hpp"
#include "decompressor_zstd.hpp"
#include "decompressor_gzip.hpp"
#include "decompressor_bzip2.hpp"

#include <cstdint>
#include <memory>

/*
 * Helper to safely check the first N bytes of the input.
 */
static inline std::uint8_t byte_or_zero(const std::vector<std::uint8_t>& v, std::size_t idx)
{
    return (idx < v.size()) ? v[idx] : 0;
}

std::unique_ptr<Decompressor> DecompressorFactory::create(const std::vector<std::uint8_t>& input)
{
    const std::uint8_t b0 = byte_or_zero(input, 0);
    const std::uint8_t b1 = byte_or_zero(input, 1);
    const std::uint8_t b2 = byte_or_zero(input, 2);
    const std::uint8_t b3 = byte_or_zero(input, 3);

    // gzip magic: 0x1F 0x8B
    if (b0 == 0x1f && b1 == 0x8b) {
        return std::unique_ptr<Decompressor>(new GzipDecompressor());
    }

    // bzip2 magic: 'B' 'Z' 'h'
    if (b0 == 'B' && b1 == 'Z' && b2 == 'h') {
        return std::unique_ptr<Decompressor>(new Bzip2Decompressor());
    }

    // Default: assume zstd.
    return std::unique_ptr<Decompressor>(new ZstdDecompressor());
}
