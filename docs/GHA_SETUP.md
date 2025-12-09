# GitHub Actions CI Setup

This document describes how Continuous Integration (CI) is configured for
the project using GitHub Actions.

## 1. CI Goals

The CI pipeline is responsible for:

1. Installing all required system dependencies.
2. Configuring and building the project using Meson and Ninja.
3. Running unit tests via `meson test`.

This ensures that each change is automatically validated before being
merged into the main branch.

## 2. Workflow Location

The CI workflow is defined in:

```text
.github/workflows/ci.yml
```

## 3. Triggers

The `ci.yml` file configures the workflow to run in two scenarios:

```yaml
on:
  push:
    branches: [ main, master ]
  pull_request:
    branches: [ main, master ]
```

Therefore, the workflow runs:

- On every push to `main` or `master`.
- On every Pull Request targeting `main` or `master`.

This matches a typical branch-based workflow:

- Development happens on feature branches.
- Changes are proposed via PRs into `main`.
- CI validates every PR automatically.

## 4. Runner Environment

The job runs on:

```yaml
runs-on: ubuntu-22.04
```

This provides a Linux environment similar to the local development setup
(WSL Ubuntu 22.04).

## 5. Main Steps of the Workflow

### 5.1 Repository Checkout

```yaml
- name: Checkout repository
  uses: actions/checkout@v4
```

Checks out the repository so the following steps can operate on the code.

### 5.2 Installing System Dependencies

```yaml
- name: Install system dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y       build-essential       meson       ninja-build       pkg-config       libunwind-dev       libgstreamer1.0-dev       gstreamer1.0-tools       libzstd-dev       zlib1g-dev       libbz2-dev       libgtest-dev       cmake
```

Key packages:

- Build tools: `build-essential`, `meson`, `ninja-build`, `pkg-config`, `cmake`
- GStreamer: `libgstreamer1.0-dev`, `gstreamer1.0-tools`, `libunwind-dev`
- Compression libs: `libzstd-dev`, `zlib1g-dev`, `libbz2-dev`
- Google Test: `libgtest-dev` (source only)

### 5.3 Building and Installing Google Test

```yaml
- name: Build and install GoogleTest
  run: |
    cd /usr/src/googletest
    sudo cmake -S . -B build
    sudo cmake --build build
    sudo cmake --install build
```

Rationale:

- On Ubuntu, `libgtest-dev` installs Google Test sources but not prebuilt
  libraries.
- This step compiles and installs `libgtest` and `libgtest_main`, so they
  can be linked with `-lgtest -lgtest_main`.

### 5.4 Meson Configuration

```yaml
- name: Configure Meson
  run: |
    meson setup builddir
```

- Creates the `builddir` build directory.
- Processes `meson.build` and `meson_options.txt`.
- Resolves dependencies, including the fallback logic for `bzip2`.

### 5.5 Build

```yaml
- name: Build
  run: |
    meson compile -C builddir
```

- Builds:
  - the GStreamer plugin shared library (`libgstzstddec.so`),
  - the unit test executable (`zstddec_tests`).

### 5.6 Test Execution

```yaml
- name: Run unit tests
  run: |
    meson test -C builddir --print-errorlogs
```

- Runs the tests defined under `tests/meson.build`.
- Fails the job if any test fails.
- Prints test error logs for debugging.

## 6. Interpreting CI Status

In the **Actions** tab:

- A green check indicates that all steps completed successfully.
- A red cross indicates that a step failed. Clicking on the job shows
  detailed logs, including which command failed and why.

In the context of a Pull Request, the CI status is shown directly on the
PR page, making it easy to see whether a change is safe to merge.

## 7. Possible Extensions

Since the CI is driven by Meson and standard Ubuntu packages, it is easy
to extend the setup in the future to:

- Add jobs on additional Ubuntu versions or platforms.
- Integrate static analysis tools (e.g., clang-tidy).
- Generate documentation as part of the pipeline (e.g., Doxygen) as a
  separate job or as an optional step.
