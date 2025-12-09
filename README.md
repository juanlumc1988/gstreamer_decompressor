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

## 0. Environment Setup (Required)

Before building the project, you must install all required system dependencies
(GStreamer development libraries, Meson, Ninja, compression libraries, GoogleTest, etc.).

This repository provides a setup script that prepares a full Ubuntu/WSL environment
in a safe and repeatable way.

### Run the initial setup script

```bash
cd scripts/
./initial_setup_ubuntu.sh
```

### If the script is not executable
```bash
chmod +x scripts/initial_setup_ubuntu.sh
./scripts/initial_setup_ubuntu.sh
```

---

### 1. Configure the build directory

```bash
meson setup builddir
```

---

## 2. Installing the Plugin

Before testing any pipeline, install the plugin into a directory scanned by GStreamer.

### **2.1 Build**

```bash
meson setup builddir
meson compile -C builddir
```

The plugin will be generated at:

```
builddir/src/libgstzstddec.so
```

### **2.2 Install locally**

```bash
mkdir -p ~/.local/lib/gstreamer-1.0
cp builddir/src/libgstzstddec.so ~/.local/lib/gstreamer-1.0/
export GST_PLUGIN_PATH="$HOME/.local/lib/gstreamer-1.0:${GST_PLUGIN_PATH:-}"
```

### **2.3 Verify installation**

```bash
gst-inspect-1.0 zstddec
```

Expected output: plugin metadata and factory description.

---

## 3. Preparing Test Files

To fully reproduce and validate plugin behavior, generate compressed versions of a sample file.

### Create a sample input file

```bash
echo "Sample test text for zstd, gzip and bzip2." > file.txt
```

### Create Zstandard compressed file

```bash
zstd -q file.txt -o file.txt.zst
```

### Create gzip compressed file

```bash
gzip -c file.txt > file.txt.gz
```

### Create bzip2 compressed file

```bash
bzip2 -c file.txt > file.txt.bz2
```

Verify:

```bash
ls file.txt*
```

You should see:

```
file.txt
file.txt.zst
file.txt.gz
file.txt.bz2
```

---

## 4. Usage Examples (Complete Pipelines)

The `zstddec` element **automatically detects** the input format, so the pipeline structure never changes.

---

### **4.1 Zstandard (.zst)**

```bash
gst-launch-1.0     filesrc location=file.txt.zst !     zstddec !     filesink location=out_zstd.txt
```

Validation:

```bash
zstd -d file.txt.zst -o ref_zstd.txt
diff -u ref_zstd.txt out_zstd.txt
```

---

### **4.2 gzip (.gz)**

```bash
gst-launch-1.0     filesrc location=file.txt.gz !     zstddec !     filesink location=out_gzip.txt
```

Validation:

```bash
gzip -dc file.txt.gz > ref_gzip.txt
diff -u ref_gzip.txt out_gzip.txt
```

---

### **4.3 bzip2 (.bz2)**

```bash
gst-launch-1.0     filesrc location=file.txt.bz2 !     zstddec !     filesink location=out_bzip2.txt
```

Validation:

```bash
bzip2 -dc file.txt.bz2 > ref_bzip2.txt
diff -u ref_bzip2.txt out_bzip2.txt
```

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
cd docs/
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

---

## Additional Documentation

| Document | Description |
|----------|-------------|
| `docs/ARCHITECTURE.md` | Architecture and data flow |
| `docs/CODE_DECISIONS.md` | Design rationale |
| `docs/TROUBLESHOOTING.md` | Common issues and fixes |
| `docs/GHA_SETUP.md` | CI explanation |

---

## Author

Juan Luis Montes Calvo  
juanluismontescalvo@icloud.com
