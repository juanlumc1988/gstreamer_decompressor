# Architecture of the `zstddec` GStreamer Plugin

## 1. Overview

The `zstddec` plugin is a GStreamer element responsible for **decompressing compressed byte streams** and outputting a raw, uncompressed stream.

Supported formats:

- **Zstandard (.zst)**
- **gzip (.gz)**
- **bzip2 (.bz2)**

The element exposes:

- **One sink pad**: receives compressed data.  
- **One src pad**: outputs decompressed data.

It is designed primarily for file-based pipelines, where the stream ends with EOS.

Example baseline usage:

```bash
gst-launch-1.0 filesrc location=file.txt.zst ! zstddec ! filesink location=file.txt
```

---

## 2. Role in a GStreamer Pipeline

A typical pipeline:

```
filesrc -> zstddec -> filesink
```

- `filesrc`: Reads a compressed file and pushes `GstBuffer`s downstream.
- `zstddec`:  
  - Accumulates incoming compressed data.  
  - Detects compression type on EOS.  
  - Decompresses using the correct backend.  
  - Pushes a single buffer of uncompressed data.  
- `filesink`: Writes output to disk.

The element is stateless between streams and resets fully after EOS.

---

## 3. Internal Architecture

### 3.1 `GstZstdDec` Element

Located in:

- `zstddec_element.hpp`
- `zstddec_element.cpp`

Responsibilities:

- Defines sink and src pad templates.
- Buffers incoming compressed data.
- On EOS:
  - Detects the format.
  - Instantiates the correct decompressor backend.
  - Produces a decompressed buffer and pushes it downstream.

### 3.2 `Decompressor` Interface

Located in:

- `decompressor.hpp`

Defines:

```cpp
virtual bool decompress(const std::vector<uint8_t>& input,
                        std::vector<uint8_t>& output) = 0;
```

Why an interface?

- Allows replacing decompression algorithms without touching the GStreamer layer.
- Allows isolated Google Test unit tests.

### 3.3 Concrete Decompressors

Each stored in its own header:

- `decompressor_zstd.hpp`
- `decompressor_gzip.hpp`
- `decompressor_bzip2.hpp`

Libraries used:

- Zstd → `libzstd`
- gzip → `zlib`
- bzip2 → `libbz2`

Each provides:
- one-shot buffer-to-buffer decompression,
- safe return semantics (`true`/`false`, not exceptions),
- internal buffer resizing if required.

### 3.4 `DecompressorFactory`

Located in:

- `decompressor_factory.hpp`
- `decompressor_factory.cpp`

Purpose:

- Read magic bytes and choose backend:
  - gzip → `1F 8B`
  - bzip2 → `42 5A 68` ("BZh")
  - else → assume Zstandard

This isolates all detection logic outside the GStreamer element.

---

## 4. Data Flow

1. **Receive buffer (Chain function)**  
   - Map the buffer.  
   - Append bytes to internal `input_data`.  
   - Unmap and unref.  

2. **Receive EOS (Sink event)**  
   - If no data: propagate EOS.  
   - Else:
     - Detect format.  
     - Create decompressor.  
     - Decompress `input_data`.  
     - Allocate output `GstBuffer`.  
     - Push downstream.  
     - Send EOS.  

3. **Reset internal state for next stream**

---

## 5. Summary

This architecture:

- cleanly separates GStreamer integration from decompression logic,
- supports multiple formats transparently,
- allows full unit testing of decompression behavior,
- integrates easily into any GStreamer pipeline,
- supports reproducible installation and verification steps.

The project is designed to be extendable, testable, and robust enough for real-world usage scenarios.
