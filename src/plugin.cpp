#include <gst/gst.h>
#include "zstddec_element.hpp"

/* In the classic GStreamer build system (Autotools), PACKAGE is defined
 * via config.h. Here we are using Meson, so we define it ourselves.
 */
#ifndef PACKAGE
#define PACKAGE "zstddec"
#endif

/**
 * @brief Plugin init function called by GStreamer at plugin load time.
 *
 * This function registers the zstddec element type in the GStreamer registry
 * and associates it with the element factory name "zstddec".
 *
 * @param plugin Pointer to the GstPlugin being initialized.
 * @return TRUE if registration succeeds, FALSE otherwise.
 */
static gboolean plugin_init(GstPlugin *plugin)
{
    /* Register the element type under the factory name "zstddec". */
    return gst_element_register(
        plugin,
        "zstddec",          // factory name used in pipelines
        GST_RANK_PRIMARY,   // rank
        GST_TYPE_ZSTDDEC);  // GType of our element
}

/* Plugin definition macro.
 * This exposes the plugin to GStreamer with metadata such as name, version,
 * license and origin.
 */
GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    zstddec,                                   // plugin name
    "Zstandard/gzip/bzip2 decoder plugin",     // description
    plugin_init,                               // init function
    "0.1.0",                                   // version
    "LGPL",                                    // license
    PACKAGE,                                   // package name / module
    "https://example.com/"                     // origin (placeholder)
)
