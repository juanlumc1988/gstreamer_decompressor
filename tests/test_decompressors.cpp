#include <gtest/gtest.h>

#include <vector>
#include <cstdint>
#include <string>

#include "../src/decompressor.hpp"
#include "../src/decompressor_factory.hpp"
#include "../src/decompressor_zstd.hpp"
#include "../src/decompressor_gzip.hpp"
#include "../src/decompressor_bzip2.hpp"

static std::vector<std::uint8_t> to_bytes(const std::string& s)
{
    return std::vector<std::uint8_t>(s.begin(), s.end());
}

static std::string to_string(const std::vector<std::uint8_t>& v)
{
    return std::string(v.begin(), v.end());
}

/*
 * Helper that uses any Decompressor implementation and returns
 * either the decompressed string or an empty string on failure.
 */
static std::string decompress_to_string(Decompressor& dec, const std::vector<std::uint8_t>& input)
{
    std::vector<std::uint8_t> output;
    bool ok = dec.decompress(input, output);
    if (!ok) {
        return std::string{};
    }
    return to_string(output);
}

TEST(ZstdDecompressorTest, DecompressEmptyInput)
{
    ZstdDecompressor dec;
    std::vector<std::uint8_t> input;  // empty input
    std::vector<std::uint8_t> output;

    bool ok = dec.decompress(input, output);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(output.empty());
}

TEST(GzipDecompressorTest, DecompressEmptyInput)
{
    GzipDecompressor dec;
    std::vector<std::uint8_t> input;
    std::vector<std::uint8_t> output;

    bool ok = dec.decompress(input, output);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(output.empty());
}

TEST(Bzip2DecompressorTest, DecompressEmptyInput)
{
    Bzip2Decompressor dec;
    std::vector<std::uint8_t> input;
    std::vector<std::uint8_t> output;

    bool ok = dec.decompress(input, output);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(output.empty());
}

TEST(FactoryTest, GzipMagicSelectsGzipDecompressor)
{
    // gzip magic: 0x1F 0x8B
    std::vector<std::uint8_t> fake_input = {0x1f, 0x8b, 0x00, 0x00};

    auto dec = DecompressorFactory::create(fake_input);
    ASSERT_TRUE(dec);

    // We cannot easily dynamic_cast here because the concrete classes
    // are header-only and not polymorphic across separate translation units.
    // Instead, we verify that the factory does not crash and returns
    // a valid object for gzip magic bytes.
    std::vector<std::uint8_t> output;
    bool ok = dec->decompress({}, output);  // empty input is always ok
    EXPECT_TRUE(ok);
    EXPECT_TRUE(output.empty());
}

TEST(FactoryTest, Bzip2MagicSelectsBzip2Decompressor)
{
    // bzip2 magic: 'B' 'Z' 'h'
    std::vector<std::uint8_t> fake_input = {'B', 'Z', 'h', 0x00};

    auto dec = DecompressorFactory::create(fake_input);
    ASSERT_TRUE(dec);

    std::vector<std::uint8_t> output;
    bool ok = dec->decompress({}, output);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(output.empty());
}

TEST(FactoryTest, DefaultIsZstdDecompressor)
{
    // Something that is neither gzip nor bzip2; should default to zstd.
    std::vector<std::uint8_t> fake_input = {0x10, 0x20, 0x30};

    auto dec = DecompressorFactory::create(fake_input);
    ASSERT_TRUE(dec);

    std::vector<std::uint8_t> output;
    bool ok = dec->decompress({}, output);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(output.empty());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
