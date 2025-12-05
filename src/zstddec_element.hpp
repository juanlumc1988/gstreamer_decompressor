#pragma once

#include <gst/gst.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "decompressor_factory.hpp"

G_BEGIN_DECLS

#define GST_TYPE_ZSTDDEC (gst_zstddec_get_type())

G_DECLARE_FINAL_TYPE(GstZstdDec, gst_zstddec, GST, ZSTDDEC, GstElement)

G_END_DECLS

/*
 * Instance and class structures.
 * Now we add internal C++ state:
 *  - sinkpad/srcpad: GStreamer pads
 *  - input_data: accumulated compressed bytes
 *  - dec: decompressor selected by the factory (zstd/gzip/bzip2)
 */

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
