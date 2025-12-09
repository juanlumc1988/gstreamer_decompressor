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
