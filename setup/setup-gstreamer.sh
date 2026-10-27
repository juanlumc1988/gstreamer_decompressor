#!/usr/bin/env bash
###############################################################################
# setup-gstreamer.sh
#
# Prepares an Ubuntu 24.04 LTS distro running on WSL2 to build, test, debug and
# document the "zstddec" GStreamer plugin (juanlumc1988/gstreamer_decompressor),
# working from VSCode on Windows (Remote - WSL extension).
#
# Standalone: it does not need to live inside the repository.
#
# Usage (from inside WSL, as a NORMAL user - do not use sudo):
#   bash setup-gstreamer.sh [--repo DIR] [--no-docs] [--no-verify] [-h|--help]
#
# What it does:
#   1) Preflight checks (not root, Ubuntu/WSL2 detection, repo location)
#   2) Installs apt packages: toolchain, Meson/Ninja, GStreamer dev, zstd/zlib/
#      bzip2 dev, GoogleTest, gdb/valgrind and (optional) doxygen/graphviz
#   3) Checks that GoogleTest links the way tests/meson.build expects
#   4) If the project is found (current directory, or --repo DIR), verifies it
#      end to end: meson setup + compile + unit tests + real gst-launch
#      pipelines for .zst/.gz/.bz2. It builds into <project>/builddir.
#
# Idempotent: safe to re-run. It never edits dotfiles or system configuration.
###############################################################################

# Packages (Ubuntu 24.04 names).
readonly BASE_PACKAGES=(
  # Toolchain
  build-essential git pkg-config meson ninja-build cmake file
  # Debugging (libc6-dbg: needed by valgrind on Ubuntu)
  gdb valgrind libc6-dbg
  # GStreamer (libunwind-dev is a hard dependency of libgstreamer1.0-dev)
  libgstreamer1.0-dev gstreamer1.0-tools libunwind-dev
  # Compression libraries and CLI tools used by the README test flow
  libzstd-dev zstd zlib1g-dev libbz2-dev bzip2 gzip
  # Unit tests: 24.04 ships prebuilt libgtest.a / libgtest_main.a
  libgtest-dev
)
readonly DOC_PACKAGES=(doxygen graphviz)
readonly REPO_URL="https://github.com/juanlumc1988/gstreamer_decompressor.git"

# Overridable only to allow testing the detection helpers with fixtures.
PROC_VERSION_FILE="${PROC_VERSION_FILE:-/proc/version}"
OS_RELEASE_FILE="${OS_RELEASE_FILE:-/etc/os-release}"

REPO_ARG="."
REPO_DIR=""
BUILD_DIR=""
INSTALL_DOCS=1
VERIFY=1
TMP_DIR=""

log()  { printf '[INFO] %s\n' "$*"; }
warn() { printf '[WARN] %s\n' "$*" >&2; }
die()  { printf '[ERROR] %s\n' "$*" >&2; exit 1; }

cleanup() {
  if [[ -n "${TMP_DIR}" ]]; then
    rm -rf -- "${TMP_DIR}"
  fi
}

usage() {
  cat <<'EOF'
Usage: bash setup-gstreamer.sh [options]

Options:
  --repo DIR    Project directory to verify (default: current directory).
                If it does not contain the zstddec project, only the
                dependencies are installed.
  --no-docs     Do not install doxygen/graphviz.
  --no-verify   Skip the final build + unit tests + pipeline smoke test.
  -h, --help    Show this help.
EOF
}

parse_args() {
  while (($# > 0)); do
    case "$1" in
      --repo)
        (($# >= 2)) || die "--repo requires a directory argument."
        REPO_ARG="$2"
        shift
        ;;
      --no-docs)   INSTALL_DOCS=0 ;;
      --no-verify) VERIFY=0 ;;
      -h|--help)   usage; exit 0 ;;
      *)           usage >&2; die "Unknown option: $1" ;;
    esac
    shift
  done
  [[ -d "${REPO_ARG}" ]] || die "Directory not found: ${REPO_ARG}"
  REPO_DIR="$(cd -- "${REPO_ARG}" && pwd -P)"
  BUILD_DIR="${REPO_DIR}/builddir"
}

# True if REPO_DIR looks like the zstddec Meson project.
project_present() {
  [[ -f "${REPO_DIR}/meson.build" ]] \
    && grep -q "'zstddec'" "${REPO_DIR}/meson.build"
}

###############################################################################
# 1. Preflight
###############################################################################
# Prints one field (e.g. ID, VERSION_ID) of the os-release file; empty if absent.
os_release_field() {
  # shellcheck source=/dev/null
  (. "${OS_RELEASE_FILE}" && printf '%s' "${!1:-}")
}

check_os() {
  local id="" version=""
  if [[ -r "${OS_RELEASE_FILE}" ]]; then
    id="$(os_release_field ID)"
    version="$(os_release_field VERSION_ID)"
  fi
  if [[ "${id}" != "ubuntu" ]]; then
    warn "Distro '${id:-unknown}' is not Ubuntu; package names may differ."
  elif [[ "${version}" != "24.04" ]]; then
    warn "Ubuntu ${version} detected; this script is validated on 24.04 only."
  else
    log "Ubuntu 24.04 detected."
  fi
}

check_wsl() {
  local kernel=""
  if [[ -r "${PROC_VERSION_FILE}" ]]; then
    kernel="$(<"${PROC_VERSION_FILE}")"
  fi
  kernel="${kernel,,}"
  if [[ "${kernel}" == *microsoft* ]]; then
    if [[ "${kernel}" == *wsl2* ]]; then
      log "WSL2 detected."
    else
      warn "WSL detected but it does not look like WSL2 (gdb/valgrind need a real"
      warn "Linux kernel). From PowerShell: wsl --set-version <distro> 2"
    fi
  else
    warn "Not running under WSL; continuing (packages are plain Ubuntu ones)."
  fi
}

warn_if_windows_fs() {
  case "$1" in
    /mnt/[a-zA-Z]|/mnt/[a-zA-Z]/*)
      warn "The repository is on the Windows filesystem (${1})."
      warn "I/O is much slower there and file modes/CRLF can break builds."
      warn "Clone it inside the WSL filesystem instead (e.g. ~/src)."
      ;;
  esac
}

preflight() {
  if [[ "${EUID}" -eq 0 ]]; then
    die "Do not run this as root/sudo: build artifacts would end up root-owned." \
        "Run it as your normal user; sudo is invoked only where needed."
  fi
  command -v apt-get >/dev/null 2>&1 || die "apt-get not found: Ubuntu/Debian required."
  command -v sudo    >/dev/null 2>&1 || die "sudo not found."
  check_os
  check_wsl
  if project_present; then
    warn_if_windows_fs "${REPO_DIR}"
  fi
  log "Requesting sudo credentials (asked once, up front)..."
  sudo -v
}

###############################################################################
# 2. Packages
###############################################################################
install_packages() {
  local pkgs=("${BASE_PACKAGES[@]}")
  if ((INSTALL_DOCS)); then
    pkgs+=("${DOC_PACKAGES[@]}")
  fi

  log "Updating package index..."
  if ! sudo apt-get update; then
    warn "apt-get update failed. On WSL2 this is usually DNS or a corporate"
    warn "proxy/VPN: check /etc/resolv.conf, configure the proxy under"
    warn "/etc/apt/apt.conf.d/, or run 'wsl --shutdown' in PowerShell and retry."
    die "Cannot update the package index."
  fi

  log "Installing ${#pkgs[@]} packages..."
  sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y "${pkgs[@]}"
}

###############################################################################
# 3. GoogleTest link probe (same flags as tests/meson.build)
###############################################################################
check_gtest() {
  log "Checking GoogleTest links with -lgtest -lgtest_main -pthread..."
  local src="${TMP_DIR}/gtest_probe.cpp" exe="${TMP_DIR}/gtest_probe"
  printf '#include <gtest/gtest.h>\nTEST(Probe, Ok) { EXPECT_EQ(1, 1); }\n' > "${src}"
  g++ -std=c++20 "${src}" -o "${exe}" -lgtest -lgtest_main -pthread \
    || die "GoogleTest does not link. Check that libgtest-dev installed correctly."
  "${exe}" >/dev/null || die "GoogleTest probe built but failed to run."

  # The old initial_setup_ubuntu.sh looked for /usr/lib/libgtest.a, which 24.04
  # does not have (it is under /usr/lib/<triplet>/), so it always rebuilt
  # GoogleTest into /usr/local. That copy shadows the apt one and goes stale
  # when libgtest-dev is updated.
  if [[ -e /usr/local/lib/libgtest.a ]]; then
    warn "A second GoogleTest is installed under /usr/local (left by the old"
    warn "initial_setup_ubuntu.sh); it shadows the libgtest-dev one. To remove it:"
    warn "  sudo rm -rf /usr/local/lib/libgtest*.a /usr/local/lib/libgmock*.a \\"
    warn "    /usr/local/include/gtest /usr/local/include/gmock \\"
    warn "    /usr/local/lib/cmake/GTest /usr/local/lib/pkgconfig/gtest*.pc \\"
    warn "    /usr/local/lib/pkgconfig/gmock*.pc"
  fi
}

report_versions() {
  log "Toolchain summary:"
  printf '  %-10s %s\n' \
    g++        "$(g++ --version | sed -n '1p')" \
    meson      "$(meson --version)" \
    ninja      "$(ninja --version)" \
    gstreamer  "$(pkg-config --modversion gstreamer-1.0)" \
    libzstd    "$(pkg-config --modversion libzstd)" \
    zlib       "$(pkg-config --modversion zlib)" \
    libbz2     "$(dpkg-query -W -f='${Version}' libbz2-dev)" \
    gtest      "$(dpkg-query -W -f='${Version}' libgtest-dev)"
}

###############################################################################
# 4. Project verification (README flow, without touching ~/.local or dotfiles)
###############################################################################
build_and_test() {
  log "Configuring Meson in ${BUILD_DIR}..."
  if [[ -f "${BUILD_DIR}/meson-private/coredata.dat" ]]; then
    (cd "${REPO_DIR}" && meson setup --reconfigure builddir)
  else
    (cd "${REPO_DIR}" && meson setup builddir)
  fi
  log "Compiling..."
  meson compile -C "${BUILD_DIR}"
  log "Running unit tests..."
  meson test -C "${BUILD_DIR}" --print-errorlogs
}

check_pipelines() {
  local plugin_dir="${BUILD_DIR}/src"
  local work="${TMP_DIR}/pipelines"
  local ext

  [[ -f "${plugin_dir}/libgstzstddec.so" ]] \
    || die "Plugin not found at ${plugin_dir}/libgstzstddec.so after build."
  mkdir -p "${work}"

  log "Running gst-launch pipelines for zstd, gzip and bzip2..."
  printf 'Sample test text for zstd, gzip and bzip2.\n' > "${work}/file.txt"
  zstd -q "${work}/file.txt" -o "${work}/file.txt.zst"
  gzip  -c "${work}/file.txt" > "${work}/file.txt.gz"
  bzip2 -c "${work}/file.txt" > "${work}/file.txt.bz2"

  # Scoped to this process: private plugin path and registry cache.
  export GST_PLUGIN_PATH="${plugin_dir}${GST_PLUGIN_PATH:+:${GST_PLUGIN_PATH}}"
  export GST_REGISTRY_1_0="${work}/registry.bin"

  gst-inspect-1.0 zstddec >/dev/null || die "gst-inspect-1.0 cannot load zstddec."
  for ext in zst gz bz2; do
    gst-launch-1.0 -q filesrc location="${work}/file.txt.${ext}" \
        ! zstddec ! filesink location="${work}/out.${ext}" \
      || die "Pipeline failed for .${ext}."
    cmp -s "${work}/file.txt" "${work}/out.${ext}" \
      || die "Output of the .${ext} pipeline differs from the original."
    log "  .${ext}: OK"
  done
}

verify_project() {
  if ! project_present; then
    warn "zstddec project not found in ${REPO_DIR}; skipping verification."
    warn "Clone it and re-run from its root (or use --repo DIR) to verify."
    return 0
  fi
  build_and_test
  check_pipelines
}

###############################################################################
# 5. Next steps
###############################################################################
print_next_steps() {
  local project_dir="${REPO_DIR}"
  local clone_step=""
  if ! project_present; then
    # Display text only, pasted later into a shell. \$HOME and not ~: the
    # GST_PLUGIN_PATH line below is double-quoted, where ~ does not expand.
    project_dir="\$HOME/src/gstreamer_decompressor"
    clone_step="  0. Clone it INSIDE the WSL filesystem (not /mnt/c):
       mkdir -p ~/src && git clone ${REPO_URL} ${project_dir}
"
  fi

  cat <<EOF

[INFO] Setup finished.

Next steps (VSCode on Windows 10 + WSL2):
${clone_step}  1. Windows side, once: install VSCode and the "WSL" extension
     (ms-vscode-remote.remote-wsl).
  2. From this WSL terminal:   cd ${project_dir} && code .
     (first run downloads vscode-server into WSL automatically)
  3. In the WSL window, install the extensions "C/C++" (ms-vscode.cpptools)
     and "Meson" (mesonbuild.mesonbuild) -> "Install in WSL".
  4. Build / test:
       meson setup builddir      # first time only
       meson compile -C builddir
       meson test -C builddir --print-errorlogs
  5. Use the plugin from any terminal without installing it:
       export GST_PLUGIN_PATH="${project_dir}/builddir/src:\${GST_PLUGIN_PATH:-}"
       gst-inspect-1.0 zstddec
EOF
  if ! command -v code >/dev/null 2>&1; then
    warn "'code' is not in the WSL PATH. Install VSCode on Windows with the"
    warn "'Add to PATH' option and make sure WSL interop is enabled"
    warn "([interop] appendWindowsPath=true in /etc/wsl.conf)."
  fi
}

main() {
  set -Eeuo pipefail
  trap 'printf "[ERROR] Failed at line %s: %s\n" "${LINENO}" "${BASH_COMMAND}" >&2' ERR
  trap cleanup EXIT

  parse_args "$@"
  TMP_DIR="$(mktemp -d)"

  preflight
  install_packages
  check_gtest
  report_versions
  if ((VERIFY)); then
    verify_project
  fi
  print_next_steps
}

# Allow sourcing (for tests) without running.
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
