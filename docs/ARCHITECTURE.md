# Architecture of the `zstddec` GStreamer Plugin

## 1. Overview

The `zstddec` plugin is a GStreamer element whose responsibility is to
**decompress compressed byte streams** and output a plain, uncompressed
byte stream.

Supported formats:

- Zstandard (zstd)
- gzip
- bzip2

The element exposes:

- **One sink pad**: receives compressed data.
- **One src pad**: produces decompressed data.

Typical usage:

```bash
gst-launch-1.0 filesrc location=file.txt.zst ! zstddec ! filesink location=file.txt
```

This pipeline is expected to produce the same output as:

```bash
zstd -q -d file.txt.zst -o file.txt
```

## 2. Role in a GStreamer Pipeline

A typical pipeline looks like this:

```
filesrc -> zstddec -> filesink
```

- `filesrc` reads a compressed file and pushes it downstream.
- `zstddec`:
  - accumulates compressed data,
  - detects format on EOS,
  - decompresses using the correct backend,
  - outputs a single uncompressed buffer.
- `filesink` writes the raw output.

## 3. Main Components

### 3.1 GStreamer Element: `GstZstdDec`

Defined in:

- `zstddec_element.hpp`
- `zstddec_element.cpp`

Responsibilities:

- Defines sink/src pads.
- Receives and stores incoming compressed data.
- On EOS, uses a `Decompressor` to decompress.
- Pushes a raw `GstBuffer` downstream.

### 3.2 `Decompressor` Interface

Defines a uniform contract:

```cpp
virtual bool decompress(const std::vector<uint8_t>& input,
                        std::vector<uint8_t>& output) = 0;
```

### 3.3 Concrete Implementations

- `ZstdDecompressor`
- `GzipDecompressor`
- `Bzip2Decompressor`

Each wraps the corresponding system library: `libzstd`, `zlib`, `libbz2`.

### 3.4 `DecompressorFactory`

Reads magic bytes to choose the correct decoder.

Rules:

- gzip → `1F 8B`
- bzip2 → `42 5A 68` (“BZh”)
- otherwise → zstd

## 4. Internal Data Flow

1. **Chain function** accumulates data.
2. **EOS event** triggers:
   - auto-detection,
   - decompression,
   - pushing final buffer.

## 5. Plugin Integration

The plugin is registered using `GST_PLUGIN_DEFINE` in `plugin.cpp`.

Install locally:

```bash
mkdir -p ~/.local/lib/gstreamer-1.0
cp builddir/src/libgstzstddec.so ~/.local/lib/gstreamer-1.0/
export GST_PLUGIN_PATH="$HOME/.local/lib/gstreamer-1.0:$GST_PLUGIN_PATH"
```

Verify:

```bash
gst-inspect-1.0 zstddec
```

---

## 6. Usage Examples

The `zstddec` element automatically detects format, so pipelines are identical.

### 6.1 Zstandard (.zst)

```bash
gst-launch-1.0     filesrc location=file.txt.zst !     zstddec !     filesink location=out_zstd.txt
```

Validate:

```bash
zstd -d file.txt.zst -o ref_zstd.txt
diff -u ref_zstd.txt out_zstd.txt
```

---

### 6.2 gzip (.gz)

```bash
gst-launch-1.0     filesrc location=file.txt.gz !     zstddec !     filesink location=out_gzip.txt
```

Validate:

```bash
gzip -dc file.txt.gz > ref_gzip.txt
diff -u ref_gzip.txt out_gzip.txt
```

---

### 6.3 bzip2 (.bz2)

```bash
gst-launch-1.0     filesrc location=file.txt.bz2 !     zstddec !     filesink location=out_bzip2.txt
```

Validate:

```bash
bzip2 -dc file.txt.bz2 > ref_bzip2.txt
diff -u ref_bzip2.txt out_bzip2.txt
```

---

These examples demonstrate how the plugin integrates seamlessly into
GStreamer pipelines while supporting three compression formats without
changing pipeline structure.
