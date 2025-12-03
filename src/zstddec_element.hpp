#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_ZSTDDEC (gst_zstddec_get_type())

/*
 * Declare a final (non-derivable) element type that inherits from GstElement.
 * The GObject type will be GstZstdDec and the factory name will be "zstddec".
 */
G_DECLARE_FINAL_TYPE(GstZstdDec, gst_zstddec, GST, ZSTDDEC, GstElement)

G_END_DECLS

/*
 * Instance and class structures.
 * For now we only define the pads; later we will extend this with
 * decompression-related state (buffers, factory, etc).
 */

struct _GstZstdDec {
    GstElement parent_instance;

    GstPad *sinkpad;
    GstPad *srcpad;
};

struct _GstZstdDecClass {
    GstElementClass parent_class;
};
