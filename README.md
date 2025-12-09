# zstddec – GStreamer Multi-Format Decompression Plugin  
A GStreamer decoder element capable of decompressing **Zstandard**, **gzip**, and **bzip2** streams.  
The element exposes one sink pad (compressed input) and one src pad (uncompressed output).

This project is implemented in **C++20**, built with **Meson**, and includes **Google Test**–based unit tests.  
Documentation is generated using **Doxygen**.

---

## Features

- Decompression support for:
  - **Zstandard (.zst)**
  - **gzip (.gz)**
  - **bzip2 (.bz2)**
- Automatic format detection based on magic bytes.
- Clean separation between GStreamer integration and decompression logic.
- Fully testable C++ architecture (Strategy + Factory patterns).
- CI pipeline using GitHub Actions.
- Technical documentation in Markdown and Doxygen.

The resulting GStreamer element is registered under the name:

```
zstddec
```

---

## Repository Structure

```
/
├── src/
│   ├── zstddec_element.cpp
│   ├── zstddec_element.hpp
│   ├── plugin.cpp
│   ├── decompressor.hpp
│   ├── decompressor_factory.cpp
│   ├── decompressor_factory.hpp
│   ├── decompressor_zstd.hpp
│   ├── decompressor_gzip.hpp
│   ├── decompressor_bzip2.hpp
│   └── ...
│
├── tests/
│   ├── test_decompressors.cpp
│   └── meson.build
│
├── docs/
│   ├── ARCHITECTURE.md
│   ├── CODE_DECISIONS.md
│   ├── TROUBLESHOOTING.md
│   └── GHA_SETUP.md
│
├── meson.build
├── meson_options.txt
└── README.md
```

---

## Build Instructions (Meson + Ninja)

### 1. Configure the build directory

```bash
meson setup builddir
```

### 2. Compile

```bash
meson compile -C builddir
```

This produces:

```
builddir/src/libgstzstddec.so
```

---

## Installing the Plugin Locally

```bash
mkdir -p ~/.local/lib/gstreamer-1.0
cp builddir/src/libgstzstddec.so ~/.local/lib/gstreamer-1.0/
export GST_PLUGIN_PATH="$HOME/.local/lib/gstreamer-1.0:${GST_PLUGIN_PATH:-}"
```

Verify:

```bash
gst-inspect-1.0 zstddec
```

---

## Usage Examples

### Zstandard

```bash
gst-launch-1.0 filesrc location=file.txt.zst ! zstddec ! filesink location=out.txt
```

### gzip

```bash
gst-launch-1.0 filesrc location=file.txt.gz ! zstddec ! filesink location=out.txt
```

### bzip2

```bash
gst-launch-1.0 filesrc location=file.txt.bz2 ! zstddec ! filesink location=out.txt
```

---

## Running Unit Tests

```bash
meson test -C builddir --print-errorlogs
```

---

## Generating Documentation (Doxygen)

Install:

```bash
sudo apt install doxygen
```

Generate:

```bash
doxygen Doxyfile
```

Output:

```
docs/doxygen/html/index.html
```

---

## Continuous Integration (CI)

Workflow file:

```
.github/workflows/ci.yml
```

Documentation:

- `docs/GHA_SETUP.md`

---

## Additional Documentation

| Document | Description |
|----------|-------------|
| `docs/ARCHITECTURE.md` | Architecture and data flow |
| `docs/CODE_DECISIONS.md` | Design rationale |
| `docs/TROUBLESHOOTING.md` | Common issues and fixes |
| `docs/GHA_SETUP.md` | CI explanation |

---

## License

MIT License.

---

## Author

Your Name  
Your Email  
