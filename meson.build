project(
  'zstddec',
  'cpp',
  version : '0.1.0',
  default_options : [
    'cpp_std=c++20',
    'warning_level=3',
  ]
)

cc = meson.get_compiler('cpp')

zstd_dep = dependency('libzstd')
gst_dep  = dependency('gstreamer-1.0')
zlib_dep = dependency('zlib')

# Intentar primero con pkg-config/cmake
bz2_dep = dependency('bzip2', required : false)

# Si falla, buscar la librería libbz2 directamente
if not bz2_dep.found()
  bz2_dep = cc.find_library('bz2', required : true)
endif

deps = [
  zstd_dep,
  gst_dep,
  zlib_dep,
  bz2_dep,
]

subdir('src')

if get_option('tests')
  subdir('tests')
endif

doxygen = find_program('doxygen', required: false)

if doxygen.found()
  doc_conf = files('docs/Doxyfile')

  custom_target(
    'doc',
    input: doc_conf,
    output: 'html',
    command: [doxygen, '@INPUT@'],
    build_by_default: false
  )
endif
