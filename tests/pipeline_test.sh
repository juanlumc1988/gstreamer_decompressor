#!/usr/bin/env bash
###############################################################################
# End-to-end test: real gst-launch-1.0 pipelines through the built plugin.
#
#   filesrc ! zstddec ! filesink
#
# Checks, for zstd, gzip and bzip2, that:
#   - the output is byte-identical to the original (including highly
#     compressible data and zstd frames that do not declare their size),
#   - corrupt or truncated input makes the pipeline FAIL (non-zero exit code)
#     instead of silently producing an empty file.
#
# Usage (run by Meson, which sets GST_PLUGIN_PATH to the build directory):
#   GST_PLUGIN_PATH=<builddir>/src tests/pipeline_test.sh
#
# Exit code 77 means "skipped" (Meson convention) when a tool is missing.
###############################################################################
set -u

for tool in gst-launch-1.0 zstd gzip bzip2 cmp; do
  if ! command -v "${tool}" >/dev/null 2>&1; then
    echo "SKIP: '${tool}' not found"
    exit 77
  fi
done

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
# Private registry: do not touch (or depend on) the user's GStreamer cache.
export GST_REGISTRY_1_0="${work}/registry.bin"

failures=0
pass() { echo "PASS  $*"; }
fail() { echo "FAIL  $*"; failures=$((failures + 1)); }

# run_pipeline <input> <output> -> exit code of gst-launch-1.0
run_pipeline() {
  gst-launch-1.0 -q filesrc location="$1" ! zstddec ! filesink location="$2" \
    >"${work}/stdout.log" 2>"${work}/stderr.log"
}

expect_identical() { # <label> <compressed> <original>
  rm -f "${work}/out.bin"
  if run_pipeline "$2" "${work}/out.bin" && cmp -s "$3" "${work}/out.bin"; then
    pass "$1"
  else
    fail "$1 (exit code or output mismatch)"
  fi
}

expect_pipeline_error() { # <label> <input>
  rm -f "${work}/out.bin"
  if run_pipeline "$2" "${work}/out.bin"; then
    fail "$1 (pipeline reported success)"
  elif grep -q "Could not decompress the stream" "${work}/stderr.log"; then
    pass "$1"
  else
    fail "$1 (failed without the expected error message)"
  fi
}

# --- Test data -------------------------------------------------------------
# Moderate ratio (~3x) and very high ratio (>>1000x).
seq 1 1000000 >"${work}/numbers.txt"
yes 'line of log text status=200 ms=12' | head -c 20000000 >"${work}/repeated.txt"
printf 'Sample test text for zstd, gzip and bzip2.\n' >"${work}/small.txt"

for name in small numbers repeated; do
  src="${work}/${name}.txt"
  zstd -q -f "${src}" -o "${work}/${name}.zst"
  zstd -q <"${src}" >"${work}/${name}.stream.zst"   # no content size in the frame
  gzip -c "${src}" >"${work}/${name}.gz"
  bzip2 -c "${src}" >"${work}/${name}.bz2"

  expect_identical "zstd  ${name}"          "${work}/${name}.zst"        "${src}"
  expect_identical "zstd  ${name} (stream)" "${work}/${name}.stream.zst" "${src}"
  expect_identical "gzip  ${name}"          "${work}/${name}.gz"         "${src}"
  expect_identical "bzip2 ${name}"          "${work}/${name}.bz2"        "${src}"
done

# Concatenated streams decompress to the concatenation.
cat "${work}/small.txt" "${work}/numbers.txt" >"${work}/concat.txt"
cat "${work}/small.gz"  "${work}/numbers.gz"  >"${work}/concat.gz"
cat "${work}/small.bz2" "${work}/numbers.bz2" >"${work}/concat.bz2"
cat "${work}/small.zst" "${work}/numbers.zst" >"${work}/concat.zst"
expect_identical "gzip  concatenated members" "${work}/concat.gz"  "${work}/concat.txt"
expect_identical "bzip2 concatenated streams" "${work}/concat.bz2" "${work}/concat.txt"
expect_identical "zstd  concatenated frames"  "${work}/concat.zst" "${work}/concat.txt"

# Empty input: nothing to decompress, EOS is forwarded, empty output.
: >"${work}/empty.bin"
rm -f "${work}/out.bin"
if run_pipeline "${work}/empty.bin" "${work}/out.bin" && [ ! -s "${work}/out.bin" ]; then
  pass "empty input -> empty output"
else
  fail "empty input -> empty output"
fi

# --- Failure cases: must be reported, not silent --------------------------
for ext in zst gz bz2; do
  size="$(stat -c %s "${work}/numbers.${ext}")"
  head -c $((size / 2)) "${work}/numbers.${ext}" >"${work}/truncated.${ext}"
  expect_pipeline_error "truncated .${ext} fails" "${work}/truncated.${ext}"
done
head -c 1000 /dev/urandom >"${work}/garbage.bin"
expect_pipeline_error "garbage input fails" "${work}/garbage.bin"

if [ "${failures}" -ne 0 ]; then
  echo "${failures} pipeline test(s) failed"
  exit 1
fi
echo "All pipeline tests passed"
