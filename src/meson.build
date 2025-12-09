# Meson build file for the zstddec plugin sources.

srcs = [
  'plugin.cpp',
  'zstddec_element.cpp',
  'decompressor_factory.cpp',
]

libgstzstddec = shared_library(
  'gstzstddec',          # name of the library: libgstzstddec.so
  srcs,
  dependencies: deps,    # inherits from root meson.build
  install: true,
  install_dir: join_paths(get_option('libdir'), 'gstreamer-1.0')
)