# Troubleshooting Guide

This document lists common problems that may occur when building or running
the project, both locally and in CI, and how to resolve them.

## 1. Build Issues

### 1.1 Meson: bzip2 dependency not found

Typical message:

```text
Run-time dependency bzip2 found: NO (tried pkgconfig and cmake)
```

Cause:

- The system provides `libbz2-dev`, but there is no `bzip2.pc` for
  pkg-config, so `dependency('bzip2')` cannot be resolved.

Solution implemented in `meson.build`:

```meson
bz2_dep = dependency('bzip2', required : false)

if not bz2_dep.found()
  cc = meson.get_compiler('cpp')
  bz2_dep = cc.find_library('bz2', required : true)
endif
```

As long as the `libbz2` library is present, the dependency is satisfied.

### 1.2 Meson: unknown option `tests`

Error:

```text
ERROR: Tried to access unknown option 'tests'.
```

Cause:

- The root `meson.build` uses:

  ```meson
  if get_option('tests')
    subdir('tests')
  endif
  ```

- but no `meson_options.txt` defining that option exists in the project root,
  or the file is ignored (e.g. due to a `*.txt` entry in `.gitignore`).

Solution:

1. Create `meson_options.txt` in the project root:

   ```meson
   option(
     'tests',
     type: 'boolean',
     value: true,
     description: 'Build and run unit tests'
   )
   ```

2. Ensure it is tracked by git, even if `.gitignore` ignores `*.txt`:

   ```bash
   git add -f meson_options.txt
   git commit -m "Add Meson tests option"
   ```

### 1.3 Linker errors with Google Test (`-lgtest`)

Symptom:

```text
/usr/bin/ld: cannot find -lgtest
```

Cause:

- On Ubuntu, `libgtest-dev` installs the Google Test sources but not the
  compiled libraries.

Solution:

```bash
sudo apt install -y libgtest-dev cmake

cd /usr/src/googletest
sudo cmake -S . -B build
sudo cmake --build build
sudo cmake --install build
```

After this, `-lgtest` and `-lgtest_main` should be resolvable.

### 1.4 Too few arguments to `gst_element_class_set_static_metadata`

Error:

```text
error: too few arguments to function
‘void gst_element_class_set_static_metadata(...)’
```

Cause:

- The call is missing the `author` argument. For example:

  ```cpp
  gst_element_class_set_static_metadata(
      element_class,
      "Zstd/gzip/bzip2 decoder",
      "Decoder/Filter",
      "Decoder for Zstandard, gzip and bzip2 compressed streams");
  ```

Solution:

- Add the author string as the fifth parameter:

  ```cpp
  gst_element_class_set_static_metadata(
      element_class,
      "Zstd/gzip/bzip2 decoder",
      "Decoder/Filter",
      "Decoder for Zstandard, gzip and bzip2 compressed streams",
      "Your Name <your.email@example.com>");
  ```

## 2. Runtime Issues

### 2.1 `gst-inspect-1.0 zstddec` cannot find the element

Symptom:

```text
No such element or plugin 'zstddec'
```

Possible causes:

- The plugin shared object (`libgstzstddec.so`) is not installed in a
  directory that GStreamer scans for plugins.
- The `GST_PLUGIN_PATH` environment variable is not set to include the
  plugin location.

Recommended local installation (per user):

```bash
mkdir -p ~/.local/lib/gstreamer-1.0
cp builddir/src/libgstzstddec.so ~/.local/lib/gstreamer-1.0/
export GST_PLUGIN_PATH="$HOME/.local/lib/gstreamer-1.0:${GST_PLUGIN_PATH:-}"

gst-inspect-1.0 zstddec
```

If `gst-inspect-1.0` still does not list `zstddec`, verify:

- the `.so` file name (`libgstzstddec.so`),
- that the library was built successfully,
- that you are not mixing 32/64-bit environments.

### 2.2 Decompressed output does not match the reference

Symptom:

```bash
diff -u ref_zstd.txt out_zstd.txt
# shows differences
```

(or for gzip/bzip2 equivalents).

Diagnostic steps:

1. Verify that the input file is valid for the expected format:

   ```bash
   file file.txt.zst
   file file.txt.gz
   file file.txt.bz2
   ```

2. Verify that the reference tools work as expected:

   ```bash
   zstd -q -d file.txt.zst -o ref_zstd.txt
   gzip -dc file.txt.gz > ref_gzip.txt
   bzip2 -dc file.txt.bz2 > ref_bzip2.txt
   ```

3. Compare the outputs:

   ```bash
   diff -u ref_zstd.txt out_zstd.txt
   diff -u ref_gzip.txt out_gzip.txt
   diff -u ref_bzip2.txt out_bzip2.txt
   ```

4. Run the GStreamer pipeline with verbose logging:

   ```bash
   gst-launch-1.0 -v filesrc location=file.txt.zst ! zstddec ! filesink location=out_zstd.txt
   ```

If the mismatch persists:

- double-check library versions (zstd, zlib, bzip2),
- ensure the plugin has been rebuilt after changes,
- check the logs for error messages in the decompression path.

## 3. CI (GitHub Actions) Issues

### 3.1 Unmet dependencies when installing `libgstreamer1.0-dev`

Example:

```text
The following packages have unmet dependencies:
 libgstreamer1.0-dev : Depends: libunwind-dev
```

Cause:

- The CI environment is missing `libunwind-dev`, which is a dependency of
  `libgstreamer1.0-dev`.

Solution:

- Add `libunwind-dev` explicitly to the list of packages installed by the
  CI workflow:

  ```yaml
  sudo apt-get install -y     libunwind-dev     libgstreamer1.0-dev     ...
  ```

### 3.2 `meson test` failures in CI

If `meson test` fails:

1. Inspect the logs of the failing job in the **Actions** tab.
2. Check:
   - whether all runtime dependencies are installed (especially Google Test),
   - that the tests do not depend on local-only paths or environment
     variables.

Aligning the CI dependency installation with the local setup script helps
avoid environment-specific differences.
