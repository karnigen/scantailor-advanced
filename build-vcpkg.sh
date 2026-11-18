#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./build-vcpkg.sh [options]

Build ScanTailor Advanced with project-local vcpkg dependencies.

Options:
  --clean             Recreate the build directory before building.
  --sync              Synchronize CMake and vcpkg even if inputs are unchanged.
  --tests             Build and run the unit tests.
  --build-type TYPE   Set CMake build type (default: Release).
  -j, --jobs N        Set parallel build jobs (default: available CPUs).
  -y, --yes           Continue without confirmation.
  -h, --help          Show this help.
EOF
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-vcpkg"
BUILD_TYPE="Release"
BUILD_TESTS="OFF"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')"
CLEAN="OFF"
ASSUME_YES="OFF"
FORCE_CONFIGURE="OFF"

while (($#)); do
  case "$1" in
    --clean)
      CLEAN="ON"
      shift
      ;;
    --sync)
      FORCE_CONFIGURE="ON"
      shift
      ;;
    --tests)
      BUILD_TESTS="ON"
      shift
      ;;
    --build-type)
      if (($# < 2)); then
        echo "Error: --build-type requires a value." >&2
        exit 2
      fi
      BUILD_TYPE="$2"
      shift 2
      ;;
    -j|--jobs)
      if (($# < 2)); then
        echo "Error: $1 requires a value." >&2
        exit 2
      fi
      JOBS="$2"
      shift 2
      ;;
    -y|--yes)
      ASSUME_YES="ON"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Error: unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

VCPKG_ROOT="${VCPKG_ROOT:-${HOME}/vcpkg}"
VCPKG_TOOLCHAIN="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
VCPKG_SIGNATURE_FILE="${BUILD_DIR}/.vcpkg-config-signature"

if [[ ! -f "$VCPKG_TOOLCHAIN" ]]; then
  echo "Error: vcpkg toolchain not found at $VCPKG_TOOLCHAIN" >&2
  echo "Set VCPKG_ROOT to the vcpkg checkout directory." >&2
  exit 1
fi

hash_file() {
  sha256sum "$1" | cut -d' ' -f1
}

CONFIG_SIGNATURE="$(
  cat <<EOF
build_type=$BUILD_TYPE
build_tests=$BUILD_TESTS
vcpkg_root=$VCPKG_ROOT
vcpkg_json=$(hash_file "$SCRIPT_DIR/vcpkg.json")
vcpkg_configuration_json=$(hash_file "$SCRIPT_DIR/vcpkg-configuration.json")
toolchain=$(hash_file "$VCPKG_TOOLCHAIN")
EOF
)"

NEEDS_CONFIGURE="OFF"
if [[ "$CLEAN" == "ON" || "$FORCE_CONFIGURE" == "ON" || ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  NEEDS_CONFIGURE="ON"
elif [[ ! -f "$VCPKG_SIGNATURE_FILE" ]] ||
     [[ "$(cat "$VCPKG_SIGNATURE_FILE")" != "$CONFIG_SIGNATURE" ]]; then
  NEEDS_CONFIGURE="ON"
fi

cat <<EOF
ScanTailor Advanced vcpkg build plan:
  Source directory:      $SCRIPT_DIR
  Build directory:       $BUILD_DIR
  vcpkg root:            $VCPKG_ROOT
  vcpkg install:         $BUILD_DIR/vcpkg_installed
  Build type:            $BUILD_TYPE
  Build tests:           $BUILD_TESTS
  Run tests after build: $BUILD_TESTS
  Parallel jobs:         $JOBS
  Clean build directory: $CLEAN
  Configure and sync:    $NEEDS_CONFIGURE

Actions:
EOF
if [[ "$NEEDS_CONFIGURE" == "ON" ]]; then
  cat <<'EOF'
  - Configure CMake with the vcpkg toolchain
  - Synchronize project dependencies under build-vcpkg
  - Build ScanTailor Advanced
EOF
else
  cat <<'EOF'
  - Skip CMake/vcpkg synchronization (use --sync to force it)
  - Ask CMake to incrementally build ScanTailor Advanced
EOF
fi
if [[ "$BUILD_TESTS" == "ON" ]]; then
  echo "  - Run CTest"
fi
if [[ "$CLEAN" == "ON" ]]; then
  echo "  Build directory will be deleted before configuration."
fi

if [[ "$ASSUME_YES" == "OFF" && -t 0 ]]; then
  read -r -p "Continue? [y/N] " REPLY
  if [[ ! "$REPLY" =~ ^([yY]|[yY][eE][sS])$ ]]; then
    echo "Build cancelled."
    exit 0
  fi
elif [[ "$ASSUME_YES" == "OFF" ]]; then
  echo "No interactive terminal detected; continuing without confirmation."
fi

if [[ "$CLEAN" == "ON" && -d "$BUILD_DIR" ]]; then
  rm -rf "$BUILD_DIR"
fi

if [[ "$NEEDS_CONFIGURE" == "ON" ]]; then
  VCPKG_FEATURES=()
  if [[ "$BUILD_TESTS" == "ON" ]]; then
    VCPKG_FEATURES=(-DVCPKG_MANIFEST_FEATURES=tests)
  fi

  cmake \
    -S "$SCRIPT_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTS="$BUILD_TESTS" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN" \
    -DVCPKG_INSTALLED_DIR="$BUILD_DIR/vcpkg_installed" \
    "${VCPKG_FEATURES[@]}"

  printf '%s\n' "$CONFIG_SIGNATURE" > "$VCPKG_SIGNATURE_FILE"
fi

cmake --build "$BUILD_DIR" --parallel "$JOBS"

if [[ "$BUILD_TESTS" == "ON" ]]; then
  ctest --test-dir "$BUILD_DIR" --build-config "$BUILD_TYPE" --output-on-failure
fi

echo "Build complete: $BUILD_DIR/scantailor-advanced"
