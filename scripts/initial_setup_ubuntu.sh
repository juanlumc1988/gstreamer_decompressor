#!/usr/bin/env bash
set -euo pipefail

###############################################################################
# Initial setup script for building the zstddec GStreamer plugin on Ubuntu.
#
# Targets:
#   - Ubuntu 22.04 / 24.04 (VM, bare metal, WSL)
#
# What this script does (idempotent as far as apt and installs go):
#   1) Updates package index (safe to re-run)
#   2) Installs build tools (compiler, Meson, Ninja, pkg-config, CMake, Git)
#   3) Installs GStreamer dev libs (including libunwind-dev for consistency with CI)
#   4) Installs compression libs/tools: zstd, zlib, bzip2
#   5) Installs debugging/doc tools: gdb, valgrind, doxygen, graphviz
#   6) Builds and installs GoogleTest only if the libraries are not already present
#
# You can run this script multiple times sin romper nada: apt-get install es
# idempotente, y la parte de GoogleTest comprueba si ya está instalado.
###############################################################################

echo "[INFO] Updating package index..."
sudo apt-get update -y

###############################################################################
# 1. Build essentials and toolchain
###############################################################################
echo "[INFO] Installing build tools and basic development packages..."

sudo apt-get install -y   build-essential   git   meson   ninja-build   pkg-config   cmake

###############################################################################
# 2. GStreamer development libraries (aligned with CI)
###############################################################################
echo "[INFO] Installing GStreamer development libraries..."

sudo apt-get install -y   libgstreamer1.0-dev   libgstreamer-plugins-base1.0-dev   gstreamer1.0-tools   libunwind-dev

###############################################################################
# 3. Compression libraries: zstd, zlib, bzip2
###############################################################################
echo "[INFO] Installing compression libraries..."

sudo apt-get install -y   libzstd-dev   zstd   zlib1g-dev   libbz2-dev   bzip2

###############################################################################
# 4. Debugging and documentation tools
###############################################################################
echo "[INFO] Installing debugging and documentation tools..."

sudo apt-get install -y   gdb   valgrind   doxygen   graphviz

###############################################################################
# 5. GoogleTest (libgtest + libgtest_main)
#
# Ubuntu instala sólo las fuentes en /usr/src/googletest al instalar libgtest-dev.
# Aquí compilamos e instalamos las libs, pero sólo si no existen ya.
###############################################################################
echo "[INFO] Installing and (if needed) building GoogleTest..."

sudo apt-get install -y libgtest-dev cmake

# Comprobar si las librerías de GoogleTest ya existen
if [ -f /usr/local/lib/libgtest.a ] || [ -f /usr/lib/libgtest.a ]; then
  echo "[INFO] GoogleTest libraries already present. Skipping manual build."
else
  echo "[INFO] GoogleTest libraries not found. Building from sources..."
  if [ -d /usr/src/googletest ]; then
    sudo cmake -S /usr/src/googletest -B /usr/src/googletest/build
    sudo cmake --build /usr/src/googletest/build
    sudo cmake --install /usr/src/googletest/build
  else
    echo "[WARN] /usr/src/googletest not found. Check libgtest-dev installation."
  fi
fi

echo
echo "[INFO] Finished initial setup."
echo "[INFO] You can now clone your repository and run Meson:"
echo "       meson setup builddir"
echo "       meson compile -C builddir"
