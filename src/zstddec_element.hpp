#pragma once

#include <gst/gst.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "decompressor_factory.hpp"

/// @brief GStreamer element that decodes zstd, gzip and bzip2 streams.
///
/// This element exposes one sink pad and one src pad. It accumulates the
/// compressed data received on the sink pad and triggers the actual
/// decompression when it receives an EOS event.
///
/// The concrete decompressor implementation (zstd, gzip or bzip2) is selected
/// at EOS time by inspecting the magic bytes and delegating to the
/// DecompressorFactory.
G_BEGIN_DECLS

#define GST_TYPE_ZSTDDEC (gst_zstddec_get_type())

G_DECLARE_FINAL_TYPE(GstZstdDec, gst_zstddec, GST, ZSTDDEC, GstElement)

G_END_DECLS

/// @brief Instance structure for the zstddec element.
///
/// It stores:
///  - The sink and src pads.
///  - An internal buffer that accumulates all compressed bytes.
///  - A unique_ptr to the selected Decompressor strategy, created lazily
///    on EOS.
struct _GstZstdDec {
    GstElement parent_instance;

    GstPad *sinkpad;
    GstPad *srcpad;

    std::vector<std::uint8_t> input_data;
    std::unique_ptr<Decompressor> dec;
};

struct _GstZstdDecClass {
    GstElementClass parent_class;
};
