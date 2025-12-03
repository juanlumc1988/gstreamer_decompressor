project(
  'zstddec',
  ['c', 'cpp'],
  version: '0.1.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3'
  ]
)

cc = meson.get_compiler('c')

# Main deps using pkg-config

zstd_dep       = dependency('libzstd', required: true)
gstreamer_dep  = dependency('gstreamer-1.0', required: true)
zlib_dep       = dependency('zlib', required: true)    
bzip2_dep = cc.find_library('bz2', required: true)

deps = [
  gstreamer_dep,
  zstd_dep,
  zlib_dep,
  bzip2_dep,
]

subdir('src')
