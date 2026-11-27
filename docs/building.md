# Building and packaging

Run commands from the repository root unless a section says otherwise.

## Release and package builds

Go to [this repository](https://github.com/ScanTailor-Advanced/scantailor-libs-build) and follow the instructions given there.

**Windows – large JPEG/PNG (issue #20):** If large-format JPEG or PNG files fail to load on Windows, ensure the build uses **libjpeg-turbo** (or libjpeg 9+) and a recent libpng. The [scantailor-libs-build](https://github.com/ScanTailor-Advanced/scantailor-libs-build) repository provides compatible libraries.

**Windows – Visual C++ runtime (issue #101):** Pre-built binaries are linked with **Microsoft Visual C++** (MSVC) runtimes. If the application does not start and Windows reports missing **`VCRUNTIME*.dll`**, **`MSVCP*.dll`**, or similar, install the **latest supported x64 “Visual C++ Redistributable”** from Microsoft’s page: [Latest supported VC++ Redistributable downloads](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170). Match **x64** vs **x86** to the build you run.

**Windows – supported OS versions (issue #101):** Release builds follow upstream **Qt** and **toolchain** support policies; **legacy Windows** (for example Windows 7) is **not** exercised in this project’s CI and is **best-effort only**. You may need an **older tagged release** or a **self-built** binary against an older Qt/MSVC stack. Community reports (including compatibility tips) are welcome in the issue tracker.


**Linux – Flatpak / Flathub ([issue #105](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/105)):**

| | |
|--|--|
| **Day-to-day binaries** | Prefer [GitHub Releases](https://github.com/ScanTailor-Advanced/scantailor-advanced/releases) when the project publishes `.deb` / AppImage (or other) builds for a tagged version. |
| **Install from Flathub** | There is **no** official Flathub listing yet for this fork under the new ID. Shipping there requires a maintainer to follow [Flathub submission](https://docs.flathub.org/docs/for-app-authors/submission/) (separate repo/PR to [flathub/flathub](https://github.com/flathub/flathub)), AppStream metainfo, screenshots, and reviewer feedback. The in-repo manifest is meant to make that step mostly mechanical; issue [#105](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/105) tracks coordination. |
| **Local Flatpak (maintainers / QA)** | Manifest: [`flatpak/org.scantailor.Advanced.json`](../flatpak/org.scantailor.Advanced.json). Add the Flathub remote, then use the staged-source command below (installs the KDE 6.11 runtime/SDK from Flathub on first run). The temporary source copy includes working-tree changes while excluding generated build directories, including versioned `build-vcpkg-qt*` directories. |
| **CI smoke build** | [`.github/workflows/flatpak.yml`](../.github/workflows/flatpak.yml) runs manually on a selected branch and uploads a **`.flatpak` bundle** as a workflow artifact. Its AppStream metadata shows the application name and version; the development entry uses the build date. This is **not** the Flathub build pipeline. |

From the repository root, run:

```bash
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
set -o pipefail
source_dir="$(mktemp -d)"
trap 'rm -rf -- "$source_dir"' EXIT
tar -C . \
  --exclude='./.git' \
  --exclude='./.flatpak-builder' \
  --exclude='./build' \
  --exclude='./build-*' \
  --exclude='./cmake-build-*' \
  --exclude='./out' \
  --exclude='./vcpkg_installed' \
  --exclude='./repo' \
  --exclude='./build-dir' \
  -cf - . | tar -xf - -C "$source_dir"
flatpak-builder --user --install-deps-from=flathub --force-clean \
  build-dir "$source_dir/flatpak/org.scantailor.Advanced.json"
```

**Application ID:** the manifest uses `org.scantailor.Advanced` so it does **not** replace the legacy `com.github._4lex4.*` Flatpak. Author docs: [for app authors](https://docs.flathub.org/docs/for-app-authors/).

**RC builds (maintainers):** `scripts/rc-build.sh` first shows the RC tag, prerelease, and builds it plans to create or reuse, then asks for confirmation. With no build options in a terminal, it offers a menu; pass only the builds needed, for example `scripts/rc-build.sh --linux deb`, `scripts/rc-build.sh --windows --macos`, or `scripts/rc-build.sh --all`. Use `-y` or `--yes` to skip the menu and confirmation and run only the actions specified by the arguments (with no build options, this creates or reuses the tag and prerelease only). RC release asset filenames include the RC suffix (for example `1.3.0-rc1`); when an RC is promoted, the workflow restores stable asset names. The internal `scripts/rename-release-assets.sh` helper is used by both workflows to plan and apply this name conversion consistently; it changes release asset names only, not package contents or their internal versions. The helper prints its rename/delete plan and asks for confirmation; workflows pass `--yes` because they run without an interactive terminal. The package-internal version remains the stable `1.3.0`. Re-running it for the same commit reuses its RC tag, so additional OS builds add assets to that release; each new commit receives the next RC number. The checkout must be clean, `gh` must be authenticated, and the RC workflow must already be on the default branch.

**GitHub Releases:** [`.github/workflows/release.yml`](../.github/workflows/release.yml) runs automatically when a `vX.Y.Z` tag is pushed (or can be run manually). The tag version must exactly match `VERSION` in `version.h.in` at that commit. If a complete RC prerelease points to the same commit, the workflow promotes that release to `vX.Y.Z` without rebuilding; otherwise it builds the Linux (DEB, RPM and AppImage), Windows, macOS and Flatpak packages, then publishes them if all builds succeed. Linux DEB packages and AppImages are built on **Ubuntu 24.04**. Treat the DEB as targeting Ubuntu 24.04 or an equivalent system-library base; it is not intended as an Ubuntu 22.04 workaround. AppImages also require a compatible **glibc** (typically **Ubuntu 24.04+** or equivalent). On older bases such as Ubuntu 22.04, build from source or use a distribution-compatible package if available. Report Linux package problems in [issue #64](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/64).

The tagged commit must contain the release workflow and reusable build workflows. For example, start the workflow for an existing tag with `gh workflow run release.yml --ref v1.3.0`; successful completion publishes a GitHub Release.

**Community examples / test data:** See also [scantailor-testing](https://github.com/ImageProcessing-ElectronicPublications/scantailor-testing) (community repository; issue [#43](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/43)).

## Building locally

### Building on Linux (Ubuntu / Debian)

Install the required dependencies:

```bash
sudo apt install build-essential cmake ninja-build libtomlplusplus-dev \
  qt6-base-dev qt6-svg-dev qt6-tools-dev \
  libjpeg-dev libpng-dev libtiff-dev zlib1g-dev
```

For a local development build, use the repository build script:

```bash
./build-vcpkg.sh
```

To build and run the unit tests, use:

```bash
./build-vcpkg.sh --tests
```

See [TESTING.md](../TESTING.md) for the test policy and details. Do not run
individual test executables or CTest directly; the build script configures,
builds, and runs the test suites.

Release builds do not target the build machine's CPU by default, so binaries can be
distributed to other compatible machines. For a GCC build used only on the same
machine, pass `-DNATIVE_CPU_OPTIMIZATIONS=ON` to CMake to enable `-march=native`
and `-mtune=native` in Release mode. Do not enable this for distributed packages.

Command-line paths:

```bash
scantailor-advanced [path]
scantailor-advanced .
scantailor-advanced /data/scans
scantailor-advanced /data/project.ScanTailor
scantailor-advanced -h
```

One existing directory starts Open/New Project workflows at that directory. One existing project file opens it directly. A missing path is rejected. Without a path, existing project/input settings and the desktop environment retain their normal behavior.

The executable will be in the `build-vcpkg` directory.

For isolated dependencies, build through vcpkg:

```bash
./build-vcpkg.sh
```

This keeps the CMake build in `build-vcpkg/` and installs non-Qt vcpkg dependencies under `$VCPKG_ROOT/installed/` (default: `~/vcpkg`), isolated by worktree. Qt must be preinstalled and is selected per user with `--qt-root` or the `QT_ROOT` environment variable; the selected path is saved in the ignored `.build-vcpkg-config`. If no kit is configured, the script fails instead of building Qt through vcpkg.
The CMake generator defaults to Ninja; install `ninja-build` to use it. Set `CMAKE_GENERATOR` to another installed
generator when needed.

To configure a preinstalled Qt 6 kit, pass its kit prefix (the `bin` directory is also accepted):

```bash
./build-vcpkg.sh --qt-root "/path/to/your/Qt/<version>/gcc_64"
```

Qt, including Qt Testlib, always comes from the selected preinstalled SDK. vcpkg supplies only the remaining project dependencies using Release libraries. Use `--sync` to force dependency synchronization or `-y` for non-interactive builds.

Running `./build-vcpkg.sh` without arguments in an interactive terminal shows the available build configurations and stored options. Each build directory stores its selection in `.build-vcpkg-config`; build directories remain ignored by Git.

To build a `.deb` package (with application icon and menu entry):

```bash
./build-deb.sh
```

This shows the build plan and asks for confirmation before compiling. Use `./build-deb.sh -y` for non-interactive builds. It produces `scantailor-advanced_<version>_<arch>.deb` in the project root. Install with `sudo dpkg -i scantailor-advanced_*.deb`.

### Building on Windows, Linux, and macOS

The **Build Windows** workflow runs manually and builds with MSVC on Windows Server 2022. The **Build macOS** workflow runs manually and builds on macOS 15 ARM64. Both use prebuilt Qt and the project's pinned vcpkg baseline for other dependencies. No local Windows or macOS setup is required.

The Windows build runs on Windows Server 2022 with prebuilt Qt 6.8.3; that verifies compilation, not runtime behavior on Windows 10/11. The application should still be smoke-tested on those desktop versions before claiming runtime compatibility.

The macOS workflow uploads a ZIP containing an ad-hoc-signed `.app` bundle with Qt dependencies. It checks the signature, runs `--help` and checks for an on-screen window without the build's Qt installation. It is an experimental ARM64 build, not notarized by Apple; Gatekeeper may block it, and it still needs interactive testing on a Mac.

For downloadable Linux packages, use the **Build Linux** workflow to create DEB, RPM, or AppImage artifacts. These Linux distribution packages use the runner's system Qt/dependencies so their package metadata and runtime linkage follow the distribution; the Windows and macOS builds use vcpkg for other dependencies.

Manual build artifact and package filenames include the application version from `version.h.in` and the short commit hash, for example `scantailor-advanced-1.3.0-abcdef0-linux-x86_64.flatpak`. Release packages use the release version without a commit hash.

Qt 6.4 is the minimum supported Qt version for Linux system packages. The macOS CI build uses prebuilt Qt 6.11.3 and targets macOS 13 or later.
