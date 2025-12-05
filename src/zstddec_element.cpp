#include "zstddec_element.hpp"

#include <gst/gst.h>
#include <cstring>  // for memcpy

G_DEFINE_TYPE(GstZstdDec, gst_zstddec, GST_TYPE_ELEMENT)

/* Forward declarations for pad functions */
static GstFlowReturn gst_zstddec_chain(GstPad *pad, GstObject *parent, GstBuffer *buffer);
static gboolean gst_zstddec_sink_event(GstPad *pad, GstObject *parent, GstEvent *event);

/* Class initialization: metadata and pad templates */
static void gst_zstddec_class_init(GstZstdDecClass *klass)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(
        element_class,
        "Zstd/gzip/bzip2 decoder",
        "Decoder/Filter",
        "Decoder for Zstandard, gzip and bzip2 compressed streams",
        "Your Name <you@example.com>");

    /* We operate on raw bytes; caps are kept generic (ANY -> ANY) */
    GstCaps *any_caps = gst_caps_new_any();

    GstPadTemplate *sink_templ = gst_pad_template_new(
        "sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS,
        any_caps);

    GstPadTemplate *src_templ = gst_pad_template_new(
        "src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        any_caps);

    gst_element_class_add_pad_template(element_class, sink_templ);
    gst_element_class_add_pad_template(element_class, src_templ);

    gst_caps_unref(any_caps);
}

/* Instance initialization: create pads and attach functions */
static void gst_zstddec_init(GstZstdDec *self)
{
    GstElementClass *klass = GST_ELEMENT_CLASS(G_OBJECT_GET_CLASS(self));

    GstPadTemplate *sink_templ =
        gst_element_class_get_pad_template(klass, "sink");
    GstPadTemplate *src_templ =
        gst_element_class_get_pad_template(klass, "src");

    self->sinkpad = gst_pad_new_from_template(sink_templ, "sink");
    self->srcpad  = gst_pad_new_from_template(src_templ, "src");

    gst_pad_set_chain_function(self->sinkpad, GST_DEBUG_FUNCPTR(gst_zstddec_chain));
    gst_pad_set_event_function(self->sinkpad, GST_DEBUG_FUNCPTR(gst_zstddec_sink_event));

    gst_element_add_pad(GST_ELEMENT(self), self->sinkpad);
    gst_element_add_pad(GST_ELEMENT(self), self->srcpad);

    self->input_data.clear();
    self->dec.reset();
}

/* Chain function: accumulate all incoming compressed data */
static GstFlowReturn gst_zstddec_chain(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    (void)pad; // avoid unused parameter warning
    GstZstdDec *self = GST_ZSTDDEC(parent);

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_buffer_unref(buffer);
        GST_ERROR_OBJECT(self, "Failed to map input buffer");
        return GST_FLOW_ERROR;
    }

    const std::uint8_t* data = map.data;
    const std::size_t   size = map.size;

    if (size > 0 && data != nullptr) {
        const std::size_t old_size = self->input_data.size();
        self->input_data.resize(old_size + size);
        std::memcpy(self->input_data.data() + old_size, data, size);
    }

    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);

    return GST_FLOW_OK;
}

/* Handle events on sink pad, particularly EOS to trigger decompression */
static gboolean gst_zstddec_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    GstZstdDec *self = GST_ZSTDDEC(parent);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS: {
        /* When EOS arrives, perform one-shot decompression of all accumulated data. */

        if (self->input_data.empty()) {
            // Nothing to decompress; just forward EOS.
            return gst_pad_event_default(pad, parent, event);
        }

        if (!self->dec) {
            self->dec = DecompressorFactory::create(self->input_data);
        }

        std::vector<std::uint8_t> output;
        bool ok = self->dec && self->dec->decompress(self->input_data, output);

        if (!ok) {
            GST_ERROR_OBJECT(self, "Decompression failed");
            // Forward EOS anyway, but no data will be pushed.
            gboolean ret = gst_pad_event_default(pad, parent, event);
            self->input_data.clear();
            self->dec.reset();
            return ret;
        }

        // Create an output GstBuffer from the decompressed data.
        GstBuffer *outbuf = gst_buffer_new_and_alloc(output.size());
        if (!outbuf) {
            GST_ERROR_OBJECT(self, "Failed to allocate output buffer");
            gboolean ret = gst_pad_event_default(pad, parent, event);
            self->input_data.clear();
            self->dec.reset();
            return ret;
        }

        GstMapInfo outmap;
        if (!gst_buffer_map(outbuf, &outmap, GST_MAP_WRITE)) {
            GST_ERROR_OBJECT(self, "Failed to map output buffer for writing");
            gst_buffer_unref(outbuf);
            gboolean ret = gst_pad_event_default(pad, parent, event);
            self->input_data.clear();
            self->dec.reset();
            return ret;
        }

        if (outmap.size >= output.size() && !output.empty()) {
            std::memcpy(outmap.data, output.data(), output.size());
        }
        gst_buffer_unmap(outbuf, &outmap);

        GstFlowReturn flow = gst_pad_push(self->srcpad, outbuf);
        if (flow != GST_FLOW_OK) {
            GST_ERROR_OBJECT(self, "Failed to push decompressed buffer: flow=%d", flow);
        }

        // Forward EOS downstream.
        gboolean ret = gst_pad_event_default(pad, parent, event);

        // Reset internal state for next stream.
        self->input_data.clear();
        self->dec.reset();

        return ret;
    }

    default:
        // For all other events, use the default handler.
        return gst_pad_event_default(pad, parent, event);
    }
}
