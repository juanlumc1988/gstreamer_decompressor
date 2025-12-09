# Design and Implementation Decisions

## 1. Language and Standard

The implementation uses **C++20**:

- `std::unique_ptr` for safe and explicit ownership.
- `std::vector<std::uint8_t>` as the canonical byte container.
- Modern initialization and type-safety improvements.

Rationale:

- Balances modern features with good support on typical Linux toolchains.
- Leads to clearer, less error-prone code than a C-style approach.

## 2. Separation of Concerns: GStreamer vs. Business Logic

Decision:

- Keep GStreamer-specific concerns (pads, buffers, events) separate from
  decompression logic (zstd/gzip/bzip2).

Implementation:

- `GstZstdDec`:
  - receives and accumulates compressed data via the sink pad,
  - reacts to EOS and pushes decompressed data via the src pad,
  - owns a `std::unique_ptr<Decompressor>`.

- The `Decompressor` family:
  - receives a `std::vector<std::uint8_t>` with compressed data,
  - produces a `std::vector<std::uint8_t>` with uncompressed data,
  - can be tested in isolation.

Benefits:

- Core decompression logic can be unit-tested with Google Test without
  needing a GStreamer runtime.
- The element remains focused on GStreamer integration only.
- The design is easier to evolve and reason about.

## 3. Patterns Used

### 3.1 Strategy: `Decompressor`

Each compression format is encapsulated in its own class implementing the
same interface:

- `Decompressor` (abstract interface)
- `ZstdDecompressor`
- `GzipDecompressor`
- `Bzip2Decompressor`

Rationale:

- Avoid complex conditional logic scattered across the element and other
  components.
- Allow new formats to be added with minimal changes to existing code.

### 3.2 Factory: `DecompressorFactory`

Instead of embedding format-detection logic in `GstZstdDec`, the project
introduces a dedicated factory:

- `DecompressorFactory::create(const std::vector<std::uint8_t>& input)`

Rationale:

- Keeps magic-byte detection and mapping to concrete types in one place.
- Simplifies the element implementation: it only requests a `Decompressor`
  when needed, and does not care which specific subclass is used.

## 4. Decompression Mode: One-Shot vs. Streaming

Decision:

- Implement decompression in **one-shot mode**, accumulating all compressed
  data until EOS and then decompressing it in a single call.

Rationale:

- Simpler state machine (no per-chunk state transitions).
- Perfectly adequate for the assignment’s file-based pipelines.
- Simplifies the interaction with underlying libraries that already
  support buffer-to-buffer operations.

Alternative (not chosen):

- Streaming decompression with incremental processing of incoming chunks.
  While more flexible for arbitrarily long streams, it requires more complex
  state management and was considered out of scope for the current goal.

## 5. Error Handling Strategy

Principles:

- Decompression functions should not throw exceptions for normal error
  conditions (corrupted data, invalid format, etc.).
- On error:
  - clear the output buffer,
  - return `false`,
  - let the GStreamer element log the error and forward EOS.

In the element:

- Errors are reported via GStreamer logging macros (e.g. `GST_ERROR_OBJECT`).
- The element always cleans up its internal state (`input_data`, `dec`)
  after EOS, regardless of success or failure of the decompression step.

## 6. System Dependencies and Meson Configuration

Decision:

- Prefer system libraries and `pkg-config` for dependency resolution, with
  fallbacks where necessary.

Details:

- `libzstd`, `gstreamer-1.0`, and `zlib` are resolved via:

  ```meson
  zstd_dep = dependency('libzstd')
  gst_dep  = dependency('gstreamer-1.0')
  zlib_dep = dependency('zlib')
  ```

- `bzip2` is resolved via:

  ```meson
  bz2_dep = dependency('bzip2', required : false)

  if not bz2_dep.found()
    cc = meson.get_compiler('cpp')
    bz2_dep = cc.find_library('bz2', required : true)
  endif
  ```

Rationale:

- This approach works even on systems where `bzip2.pc` is not available.
- Keeps the build description portable while still leveraging Meson’s
  dependency mechanisms.

## 7. Scope of Unit Tests

Decision:

- Focus unit tests on the pure C++ layer:
  - `Decompressor` interface behavior,
  - concrete decompressor implementations,
  - `DecompressorFactory`.

Rationale:

- GStreamer integration is better validated via pipeline-level tests.
- Unit tests bring the most value when they target logic that is:
  - deterministic,
  - side-effect free (apart from memory allocation),
  - independent of external processes or daemons.

Result:

- The `tests` subtree builds a `zstddec_tests` executable that verifies:
  - behavior with empty input,
  - correct handling of magic bytes by the factory,
  - basic wiring between factory-created instances and the `Decompressor`
    interface.
