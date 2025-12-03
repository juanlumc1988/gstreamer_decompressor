#!/usr/bin/env bash
set -euo pipefail

###############################################################################
# Initial setup script for building the zstddec GStreamer plugin on Ubuntu.
#
# Targets:
#   - Ubuntu 22.04 / 24.04 (VM, bare metal, WSL)
#
#   1) Updates package index
#   2) Installs build tools (compiler, Meson, Ninja, pkg-config, CMake, Git)
#   3) Installs GStreamer dev libs
#   4) Installs compression libs/tools: zstd, zlib, bzip2
#   5) Installs some useful debugging/doc tools (optional but recommended)
#
###############################################################################

echo "[INFO] Detecting OS..."

if [ -f /etc/os-release ]; then
  . /etc/os-release
  echo "[INFO] Detected: $NAME $VERSION"
else
  echo "[WARN] /etc/os-release not found. Proceeding as if this were Ubuntu."
fi

echo "[INFO] Updating package index..."
sudo apt update

echo "[INFO] Installing base build tools..."
sudo apt install -y \
  build-essential \
  meson \
  ninja-build \
  pkg-config \
  cmake \
  git

echo "[INFO] Installing GStreamer development libraries..."
sudo apt install -y \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  gstreamer1.0-tools

echo "[INFO] Installing compression libraries and tools..."
sudo apt install -y \
  libzstd-dev zstd \
  zlib1g-dev \
  libbz2-dev \
  gzip \
  bzip2

echo "[INFO] Installing optional debugging/documentation tools (nice to have)..."
sudo apt install -y \
  gdb \
  valgrind \
  doxygen \
  graphviz

echo
echo "[INFO] Finished initial setup."
echo "[INFO] You can now clone your repository and run:"
echo "       meson setup builddir"
echo "       meson compile -C builddir"
