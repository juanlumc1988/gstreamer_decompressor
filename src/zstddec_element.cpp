#include "zstddec_element.hpp"

#include <gst/gst.h>

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
        "Zstd/gzip/bzip2 decoder (passthrough skeleton)",
        "Decoder/Filter",
        "Skeleton element for zstd/gzip/bzip2 decompression plugin",
        "Your Name <you@example.com>");

    /* For now we keep caps very generic: ANY on sink and src */
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

    /* Attach chain and event functions to the sink pad */
    gst_pad_set_chain_function(self->sinkpad, GST_DEBUG_FUNCPTR(gst_zstddec_chain));
    gst_pad_set_event_function(self->sinkpad, GST_DEBUG_FUNCPTR(gst_zstddec_sink_event));

    gst_element_add_pad(GST_ELEMENT(self), self->sinkpad);
    gst_element_add_pad(GST_ELEMENT(self), self->srcpad);
}

/* Chain function: called when upstream pushes a buffer into the sink pad */
static GstFlowReturn gst_zstddec_chain(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    GstZstdDec *self = GST_ZSTDDEC(parent);

    /* For now, simply pass the buffer through unchanged */
    return gst_pad_push(self->srcpad, buffer);
}

/* Event function on sink pad (EOS, FLUSH, CAPS, etc.) */
static gboolean gst_zstddec_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    /* For now, just let the default sink-pad event handler deal with it.
     * We will override EOS explicitly later when we implement real
     * decompression logic.
     */
    return gst_pad_event_default(pad, parent, event);
}
