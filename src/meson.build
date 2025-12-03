# Meson build file for the zstddec plugin sources.

srcs = [
  'plugin.cpp',
  'zstddec_element.cpp',
]

libgstzstddec = shared_library(
  'gstzstddec',          # el nombre real del .so: libgstzstddec.so
  srcs,
  dependencies: deps,    # viene del meson.build raíz
  install: true,
  install_dir: join_paths(get_option('libdir'), 'gstreamer-1.0')
)