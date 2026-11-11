#include <gtest/gtest.h>

#include <bzlib.h>
#include <zlib.h>
#include <zstd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../src/decompressor.hpp"
#include "../src/decompressor_factory.hpp"
#include "../src/decompressor_zstd.hpp"
#include "../src/decompressor_gzip.hpp"
#include "../src/decompressor_bzip2.hpp"

using Bytes = std::vector<std::uint8_t>;

namespace {

Bytes to_bytes(const std::string& s)
{
    return Bytes(s.begin(), s.end());
}

Bytes concat(const Bytes& a, const Bytes& b)
{
    Bytes r(a);
    r.insert(r.end(), b.begin(), b.end());
    return r;
}

/* ---- Test payloads ---------------------------------------------------- */

Bytes text_payload()
{
    return to_bytes("Sample test text for zstd, gzip and bzip2.\n");
}

/// Very high compression ratio (far beyond any fixed output-size heuristic).
Bytes highly_compressible_payload()
{
    return Bytes(8u * 1024u * 1024u, 'a');
}

/// Log-like text: moderate ratio (~10x), the kind of data that used to fail.
Bytes log_like_payload()
{
    std::string s;
    for (int i = 0; i < 40000; ++i) {
        s += "2026-11-11 10:" + std::to_string(i % 60) + " INFO worker-" + std::to_string(i % 8) +
             " request id=" + std::to_string(i % 997) + " status=200\n";
    }
    return to_bytes(s);
}

/// Incompressible data (ratio ~1), deterministic.
Bytes incompressible_payload()
{
    Bytes r(1024u * 1024u);
    std::uint32_t x = 12345u;
    for (auto& b : r) {
        x = x * 1664525u + 1013904223u;
        b = static_cast<std::uint8_t>(x >> 24);
    }
    return r;
}

/* ---- Compressors (same libraries the plugin uses) ----------------------- */

Bytes zstd_compress(const Bytes& in)
{
    Bytes out(ZSTD_compressBound(in.size()));
    const std::size_t n = ZSTD_compress(out.data(), out.size(), in.data(), in.size(), 3);
    EXPECT_FALSE(ZSTD_isError(n));
    out.resize(n);
    return out;
}

/// Zstd frame WITHOUT the content size in its header (as produced when
/// compressing from a pipe): the input is fed in two steps.
Bytes zstd_compress_streaming(const Bytes& in)
{
    std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> cctx(ZSTD_createCCtx(), &ZSTD_freeCCtx);
    Bytes out(ZSTD_compressBound(in.size()) + 64);
    ZSTD_outBuffer o{out.data(), out.size(), 0};
    const std::size_t half = in.size() / 2;

    ZSTD_inBuffer first{in.data(), half, 0};
    EXPECT_FALSE(ZSTD_isError(ZSTD_compressStream2(cctx.get(), &o, &first, ZSTD_e_continue)));
    ZSTD_inBuffer second{in.data() + half, in.size() - half, 0};
    std::size_t remaining = 1;
    while (remaining != 0) {
        remaining = ZSTD_compressStream2(cctx.get(), &o, &second, ZSTD_e_end);
        EXPECT_FALSE(ZSTD_isError(remaining));
    }
    out.resize(o.pos);
    return out;
}

Bytes gzip_compress(const Bytes& in)
{
    z_stream s{};
    EXPECT_EQ(deflateInit2(&s, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY), Z_OK);
    Bytes out(deflateBound(&s, static_cast<uLong>(in.size())) + 64);
    s.next_in = const_cast<Bytef*>(in.data());
    s.avail_in = static_cast<uInt>(in.size());
    s.next_out = out.data();
    s.avail_out = static_cast<uInt>(out.size());
    EXPECT_EQ(deflate(&s, Z_FINISH), Z_STREAM_END);
    out.resize(out.size() - s.avail_out);
    deflateEnd(&s);
    return out;
}

Bytes bzip2_compress(const Bytes& in)
{
    unsigned int len = static_cast<unsigned int>(in.size() + in.size() / 100 + 1024);
    Bytes out(len);
    // libbz2 rejects a null source pointer, which is what .data() may be for
    // an empty vector.
    char empty = 0;
    char* source = in.empty() ? &empty : const_cast<char*>(reinterpret_cast<const char*>(in.data()));
    EXPECT_EQ(BZ2_bzBuffToBuffCompress(reinterpret_cast<char*>(out.data()), &len, source,
                                       static_cast<unsigned int>(in.size()), 9, 0, 0),
              BZ_OK);
    out.resize(len);
    return out;
}

/* ---- Codec-generic checks ---------------------------------------------- */

struct Codec {
    const char* name;
    Bytes (*compress)(const Bytes&);
    std::unique_ptr<Decompressor> (*make)();
};

std::unique_ptr<Decompressor> make_zstd() { return std::unique_ptr<Decompressor>(new ZstdDecompressor()); }
std::unique_ptr<Decompressor> make_gzip() { return std::unique_ptr<Decompressor>(new GzipDecompressor()); }
std::unique_ptr<Decompressor> make_bzip2() { return std::unique_ptr<Decompressor>(new Bzip2Decompressor()); }

const Codec kCodecs[] = {
    {"zstd", zstd_compress, make_zstd},
    {"gzip", gzip_compress, make_gzip},
    {"bzip2", bzip2_compress, make_bzip2},
};

}  // namespace

class CodecTest : public ::testing::TestWithParam<Codec> {};

INSTANTIATE_TEST_SUITE_P(AllCodecs, CodecTest, ::testing::ValuesIn(kCodecs),
                         [](const ::testing::TestParamInfo<Codec>& info) { return std::string(info.param.name); });

TEST_P(CodecTest, EmptyInputIsSuccessfulNoOp)
{
    Bytes output = to_bytes("stale");
    EXPECT_TRUE(GetParam().make()->decompress({}, output));
    EXPECT_TRUE(output.empty());
}

TEST_P(CodecTest, RoundTripText)
{
    const Bytes original = text_payload();
    Bytes output;
    ASSERT_TRUE(GetParam().make()->decompress(GetParam().compress(original), output));
    EXPECT_EQ(output, original);
}

TEST_P(CodecTest, RoundTripEmptyPayload)
{
    Bytes output = to_bytes("stale");
    ASSERT_TRUE(GetParam().make()->decompress(GetParam().compress({}), output));
    EXPECT_TRUE(output.empty());
}

TEST_P(CodecTest, RoundTripHighlyCompressible)
{
    const Bytes original = highly_compressible_payload();
    const Bytes compressed = GetParam().compress(original);
    ASSERT_GT(original.size() / compressed.size(), 100u) << "payload is not highly compressible";
    Bytes output;
    ASSERT_TRUE(GetParam().make()->decompress(compressed, output));
    EXPECT_EQ(output, original);
}

TEST_P(CodecTest, RoundTripLogLikeText)
{
    const Bytes original = log_like_payload();
    Bytes output;
    ASSERT_TRUE(GetParam().make()->decompress(GetParam().compress(original), output));
    EXPECT_EQ(output, original);
}

TEST_P(CodecTest, RoundTripIncompressible)
{
    const Bytes original = incompressible_payload();
    Bytes output;
    ASSERT_TRUE(GetParam().make()->decompress(GetParam().compress(original), output));
    EXPECT_EQ(output, original);
}

TEST_P(CodecTest, ConcatenatedStreamsAreAppendedInOrder)
{
    const Bytes a = text_payload();
    const Bytes b = log_like_payload();
    const Bytes both = concat(GetParam().compress(a), GetParam().compress(b));
    Bytes output;
    ASSERT_TRUE(GetParam().make()->decompress(both, output));
    EXPECT_EQ(output, concat(a, b));
}

TEST_P(CodecTest, TruncatedInputFails)
{
    Bytes compressed = GetParam().compress(log_like_payload());
    for (const std::size_t cut : {std::size_t{1}, std::size_t{5}, compressed.size() / 2}) {
        Bytes truncated(compressed.begin(), compressed.end() - static_cast<std::ptrdiff_t>(cut));
        Bytes output = to_bytes("stale");
        EXPECT_FALSE(GetParam().make()->decompress(truncated, output)) << "cut=" << cut;
        EXPECT_TRUE(output.empty()) << "cut=" << cut;
    }
}

TEST_P(CodecTest, TrailingGarbageFails)
{
    Bytes data = GetParam().compress(text_payload());
    const Bytes garbage = {0xde, 0xad, 0xbe, 0xef, 0x01, 0x02, 0x03, 0x04};
    data.insert(data.end(), garbage.begin(), garbage.end());
    Bytes output;
    EXPECT_FALSE(GetParam().make()->decompress(data, output));
    EXPECT_TRUE(output.empty());
}

TEST_P(CodecTest, CorruptedPayloadFailsOrDiffers)
{
    // Flipping bytes in the middle must never crash or hang. Codecs with an
    // integrity check (gzip CRC, bzip2 CRC) must report failure.
    const Bytes original = log_like_payload();
    Bytes data = GetParam().compress(original);
    for (std::size_t i = data.size() / 2; i < data.size() / 2 + 16; ++i) {
        data[i] ^= 0xff;
    }
    Bytes output;
    const bool ok = GetParam().make()->decompress(data, output);
    if (ok) {
        EXPECT_NE(output, original);
    } else {
        EXPECT_TRUE(output.empty());
    }
}

TEST_P(CodecTest, GarbageInputFails)
{
    const Bytes garbage = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90};
    Bytes output;
    EXPECT_FALSE(GetParam().make()->decompress(garbage, output));
    EXPECT_TRUE(output.empty());
}

/* ---- Zstd specifics ----------------------------------------------------- */

TEST(ZstdDecompressorTest, FrameWithoutContentSizeHighRatio)
{
    // The frame header does not declare the size: nothing to size the output by.
    const Bytes original = highly_compressible_payload();
    const Bytes compressed = zstd_compress_streaming(original);
    ASSERT_EQ(ZSTD_getFrameContentSize(compressed.data(), compressed.size()), ZSTD_CONTENTSIZE_UNKNOWN);
    Bytes output;
    ASSERT_TRUE(ZstdDecompressor().decompress(compressed, output));
    EXPECT_EQ(output, original);
}

TEST(ZstdDecompressorTest, FrameWithoutContentSizeLogLikeText)
{
    const Bytes original = log_like_payload();
    Bytes output;
    ASSERT_TRUE(ZstdDecompressor().decompress(zstd_compress_streaming(original), output));
    EXPECT_EQ(output, original);
}

/* ---- Factory: selects the backend by magic bytes ------------------------ */

TEST(FactoryTest, SelectsGzipForGzipData)
{
    const Bytes original = log_like_payload();
    const Bytes compressed = gzip_compress(original);
    ASSERT_EQ(compressed[0], 0x1f);
    ASSERT_EQ(compressed[1], 0x8b);
    auto dec = DecompressorFactory::create(compressed);
    ASSERT_TRUE(dec);
    Bytes output;
    ASSERT_TRUE(dec->decompress(compressed, output));
    EXPECT_EQ(output, original);
}

TEST(FactoryTest, SelectsBzip2ForBzip2Data)
{
    const Bytes original = log_like_payload();
    const Bytes compressed = bzip2_compress(original);
    ASSERT_EQ(std::string(compressed.begin(), compressed.begin() + 3), "BZh");
    auto dec = DecompressorFactory::create(compressed);
    ASSERT_TRUE(dec);
    Bytes output;
    ASSERT_TRUE(dec->decompress(compressed, output));
    EXPECT_EQ(output, original);
}

TEST(FactoryTest, SelectsZstdForZstdData)
{
    const Bytes original = log_like_payload();
    const Bytes compressed = zstd_compress(original);
    auto dec = DecompressorFactory::create(compressed);
    ASSERT_TRUE(dec);
    Bytes output;
    ASSERT_TRUE(dec->decompress(compressed, output));
    EXPECT_EQ(output, original);
}

TEST(FactoryTest, UnknownDataDefaultsToZstdAndIsRejected)
{
    const Bytes unknown = {0x10, 0x20, 0x30};
    auto dec = DecompressorFactory::create(unknown);
    ASSERT_TRUE(dec);
    Bytes output;
    EXPECT_FALSE(dec->decompress(unknown, output));
}

TEST(FactoryTest, InputShorterThanMagicDefaultsToZstd)
{
    // Too short for any magic: must not read out of bounds and must not throw.
    for (const Bytes& tiny : {Bytes{}, Bytes{0x1f}, Bytes{'B', 'Z'}}) {
        auto dec = DecompressorFactory::create(tiny);
        ASSERT_TRUE(dec);
        Bytes output;
        EXPECT_NO_THROW(dec->decompress(tiny, output));
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
