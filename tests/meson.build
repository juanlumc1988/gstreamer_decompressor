# tests/meson.build

test_srcs = [
  'test_decompressors.cpp',
]

gtest_dep = declare_dependency(
  link_args: ['-lgtest', '-lgtest_main', '-pthread']
)

decompressor_test_srcs = [
  '../src/decompressor_factory.cpp',
]

zstddec_test_exe = executable(
  'zstddec_tests',
  test_srcs + decompressor_test_srcs,
  include_directories: [include_directories('../src')],
  dependencies: [
    gtest_dep,
    zstd_dep,
    zlib_dep,
    bz2_dep,
  ],
)

test(
  'zstddec_unit_tests',
  zstddec_test_exe
)

# End-to-end test with real gst-launch-1.0 pipelines through the built plugin.
# Exit code 77 (skip) when gst-launch-1.0, zstd, gzip or bzip2 are missing.
bash = find_program('bash', required : false)
if bash.found()
  test(
    'zstddec_pipelines',
    bash,
    args : [files('pipeline_test.sh')],
    env : ['GST_PLUGIN_PATH=' + (meson.project_build_root() / 'src')],
    depends : libgstzstddec,
    timeout : 120
  )
endif
