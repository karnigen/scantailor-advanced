#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./build-vcpkg.sh [options]

Build ScanTailor Advanced with project-local vcpkg dependencies.

Options:
  --clean             Recreate the build directory before building.
  --sync              Synchronize CMake and vcpkg even if inputs are unchanged.
  --qt-root PATH      Use a local Qt kit ending in <version>/gcc_64;
                      other dependencies still come from vcpkg.
  --tests             Build and run the unit tests.
  --build-type TYPE   Set CMake build type (default: Release).
  -j, --jobs N        Set parallel build jobs (default: available CPUs).
  -y, --yes           Continue without confirmation.
  -h, --help          Show this help.
EOF
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="Release"
BUILD_TESTS="OFF"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')"
CLEAN="OFF"
ASSUME_YES="OFF"
FORCE_CONFIGURE="OFF"
QT_ROOT=""
INITIAL_ARG_COUNT=$#

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
    --qt-root)
      if (($# < 2)); then
        echo "Error: --qt-root requires a path." >&2
        exit 2
      fi
      QT_ROOT="$2"
      shift 2
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
INITIAL_VCPKG_ROOT="$VCPKG_ROOT"
VCPKG_TOOLCHAIN="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"

display_path() {
  local path="$1"
  if [[ "$path" == "$SCRIPT_DIR" ]]; then
    printf '.'
  elif [[ "$path" == "$SCRIPT_DIR/"* ]]; then
    printf '%s' "${path#"$SCRIPT_DIR"/}"
  elif [[ -n "${HOME:-}" && "$path" == "$HOME/"* ]]; then
    printf '~/%s' "${path#"$HOME"/}"
  else
    printf '%s' "$path"
  fi
}

read_build_config() {
  local config_file="$1"
  local key value

  BUILD_TYPE="Release"
  BUILD_TESTS="OFF"
  JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')"
  QT_ROOT=""
  VCPKG_ROOT="$INITIAL_VCPKG_ROOT"

  [[ -f "$config_file" ]] || return 0
  while IFS='=' read -r key value; do
    case "$key" in
      build_type) BUILD_TYPE="$value" ;;
      build_tests) BUILD_TESTS="$value" ;;
      jobs) JOBS="$value" ;;
      qt_root) QT_ROOT="$value" ;;
      vcpkg_root) VCPKG_ROOT="$value" ;;
    esac
  done < "$config_file"
}

infer_local_qt_root() {
  local build_dir="$1"
  local cache="$build_dir/CMakeCache.txt"
  local qt_dir

  [[ -f "$cache" ]] || return 1
  qt_dir="$(sed -n 's/^Qt6_DIR:.*=//p' "$cache" | head -1)"
  [[ -n "$qt_dir" ]] || return 1
  if [[ "$qt_dir" == *"/vcpkg_installed/"* || "$qt_dir" == "$SCRIPT_DIR/build-vcpkg/"* ]]; then
    return 1
  fi
  if [[ "$qt_dir" == */lib/cmake/Qt6 ]]; then
    printf '%s' "${qt_dir%/lib/cmake/Qt6}"
  fi
}

select_interactive_build() {
  local build_dirs=()
  local dir config qt_root build_type build_tests jobs label
  local index

  build_dirs+=("$SCRIPT_DIR/build-vcpkg")
  local qt_dir
  for qt_dir in "$SCRIPT_DIR"/build-vcpkg-qt*; do
    [[ -d "$qt_dir" ]] && build_dirs+=("$qt_dir")
  done
  ((${#build_dirs[@]} > 0)) || return 0

  echo "Available build configurations:"
  index=1
  for dir in "${build_dirs[@]}"; do
    qt_root=""
    build_type="Release"
    build_tests="OFF"
    jobs="$JOBS"
    config="$dir/.build-vcpkg-config"
    read_build_config "$config"
    qt_root="$QT_ROOT"
    build_type="$BUILD_TYPE"
    build_tests="$BUILD_TESTS"
    jobs="$JOBS"
    if [[ -z "$qt_root" ]]; then
      qt_root="$(infer_local_qt_root "$dir" || true)"
    fi
    if [[ -n "$qt_root" ]]; then
      label="local Qt $(display_path "$qt_root")"
    else
      label="vcpkg Qt"
    fi
    if [[ ! -d "$dir" ]]; then
      label="$label, new build"
    fi
    printf '  %d) %s (%s, tests=%s, jobs=%s)\n' \
      "$index" "$(display_path "$dir")" "$label" "$build_tests" "$jobs"
    index=$((index + 1))
  done
  echo "  0) Cancel"

  local reply
  read -r -p "Select a build configuration: " reply
  if [[ "$reply" =~ ^[0-9]+$ ]] &&
     ((reply >= 1 && reply <= ${#build_dirs[@]})); then
    dir="${build_dirs[$((reply - 1))]}"
    read_build_config "$dir/.build-vcpkg-config"
    if [[ -z "$QT_ROOT" && "$(basename "$dir")" != "build-vcpkg" ]]; then
      QT_ROOT="$(infer_local_qt_root "$dir" || true)"
    fi
  else
    echo "Build cancelled."
    exit 0
  fi
}

save_build_config() {
  local build_dir="$1"
  local config_file="$build_dir/.build-vcpkg-config"
  mkdir -p "$build_dir"
  cat > "$config_file" <<EOF
build_type=$BUILD_TYPE
build_tests=$BUILD_TESTS
jobs=$JOBS
qt_root=$QT_ROOT
vcpkg_root=$VCPKG_ROOT
EOF
}

if [[ "$INITIAL_ARG_COUNT" -eq 0 && -t 0 ]]; then
  select_interactive_build
fi
VCPKG_TOOLCHAIN="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
QT_SIGNATURE="vcpkg"

if [[ -n "$QT_ROOT" ]]; then
  QT_ROOT="$(cd "$QT_ROOT" 2>/dev/null && pwd)" || {
    echo "Error: Qt root does not exist: $QT_ROOT" >&2
    exit 1
  }
  if [[ "$QT_ROOT" == */bin ]]; then
    QT_ROOT="$(dirname "$QT_ROOT")"
  fi
  if [[ ! -f "$QT_ROOT/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
    echo "Error: invalid Qt kit prefix: $QT_ROOT" >&2
    echo "Expected <Qt-install>/<version>/gcc_64 or its bin directory." >&2
    echo "Required: $QT_ROOT/lib/cmake/Qt6/Qt6Config.cmake" >&2
    exit 1
  fi
  QT_SIGNATURE="$(sha256sum "$QT_ROOT/lib/cmake/Qt6/Qt6Config.cmake" | cut -d' ' -f1)"
  if ! QT_VERSION="$("$QT_ROOT/bin/qmake6" -query QT_VERSION)"; then
    QT_VERSION=""
  fi
  if [[ -z "$QT_VERSION" || ! "$QT_VERSION" =~ ^[0-9]+(\.[0-9]+)*$ ]]; then
    echo "Error: could not determine Qt version from: $QT_ROOT" >&2
    exit 1
  fi
  BUILD_DIR="${SCRIPT_DIR}/build-vcpkg-qt${QT_VERSION}"
  QT_SOURCE="$QT_ROOT"
else
  BUILD_DIR="${SCRIPT_DIR}/build-vcpkg"
  QT_SOURCE="vcpkg"
fi

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
qt_root=$QT_ROOT
qt_signature=$QT_SIGNATURE
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
  Source directory:      $(display_path "$SCRIPT_DIR")
  Build directory:       $(display_path "$BUILD_DIR")
  vcpkg root:            $(display_path "$VCPKG_ROOT")
  vcpkg install:         $(display_path "$BUILD_DIR/vcpkg_installed")
  Qt source:             $(display_path "$QT_SOURCE")
  Binary:                $(display_path "$BUILD_DIR/scantailor-advanced")
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
  - Synchronize project dependencies in the selected build directory
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
  CACHED_QT_DIR=""
  if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    CACHED_QT_DIR="$(sed -n 's/^Qt6_DIR:.*=//p' "$BUILD_DIR/CMakeCache.txt" | head -1)"
  fi
  if [[ -n "$QT_ROOT" ]]; then
    EXPECTED_QT_DIR="$QT_ROOT/lib/cmake/Qt6"
    if [[ -n "$CACHED_QT_DIR" && "$CACHED_QT_DIR" != "$EXPECTED_QT_DIR" ]]; then
      rm -f "$BUILD_DIR/CMakeCache.txt"
      if [[ -d "$BUILD_DIR/CMakeFiles" ]]; then
        cmake -E remove_directory "$BUILD_DIR/CMakeFiles"
      fi
    fi
  elif [[ -n "$CACHED_QT_DIR" && "$CACHED_QT_DIR" != *"/vcpkg_installed/"* ]]; then
    rm -f "$BUILD_DIR/CMakeCache.txt"
    if [[ -d "$BUILD_DIR/CMakeFiles" ]]; then
      cmake -E remove_directory "$BUILD_DIR/CMakeFiles"
    fi
  fi

  OLD_CONFIG_SIGNATURE=""
  if [[ -f "$VCPKG_SIGNATURE_FILE" ]]; then
    OLD_CONFIG_SIGNATURE="$(cat "$VCPKG_SIGNATURE_FILE")"
  fi
  OLD_QT_SIGNATURE="$(printf '%s\n' "$OLD_CONFIG_SIGNATURE" | sed -n 's/^qt_signature=//p')"
  NEW_QT_SIGNATURE="$(printf '%s\n' "$CONFIG_SIGNATURE" | sed -n 's/^qt_signature=//p')"
  if [[ -n "$OLD_QT_SIGNATURE" && "$OLD_QT_SIGNATURE" != "$NEW_QT_SIGNATURE" ]]; then
    rm -f "$BUILD_DIR/CMakeCache.txt"
    if [[ -d "$BUILD_DIR/CMakeFiles" ]]; then
      cmake -E remove_directory "$BUILD_DIR/CMakeFiles"
    fi
  fi
  rm -f "$VCPKG_SIGNATURE_FILE"

  VCPKG_MANIFEST_OPTIONS=()
  VCPKG_FEATURES=()
  CMAKE_PREFIX_OPTIONS=()
  if [[ -n "$QT_ROOT" ]]; then
    VCPKG_MANIFEST_OPTIONS=(-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON)
    CMAKE_PREFIX_OPTIONS=(-DCMAKE_PREFIX_PATH="$QT_ROOT")
  fi
  if [[ "$BUILD_TESTS" == "ON" ]]; then
    VCPKG_FEATURES=(-DVCPKG_MANIFEST_FEATURES=tests)
  fi

  cmake \
    -S "$SCRIPT_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTS="$BUILD_TESTS" \
    -DBoost_USE_STATIC_LIBS=ON \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN" \
    -DVCPKG_INSTALLED_DIR="$BUILD_DIR/vcpkg_installed" \
    "${CMAKE_PREFIX_OPTIONS[@]}" \
    "${VCPKG_MANIFEST_OPTIONS[@]}" \
    "${VCPKG_FEATURES[@]}"

  printf '%s\n' "$CONFIG_SIGNATURE" > "$VCPKG_SIGNATURE_FILE"
  save_build_config "$BUILD_DIR"
fi

cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel "$JOBS"
save_build_config "$BUILD_DIR"

if [[ "$BUILD_TESTS" == "ON" ]]; then
  ctest --test-dir "$BUILD_DIR" --build-config "$BUILD_TYPE" --output-on-failure
fi

echo "Build complete: $(display_path "$BUILD_DIR/scantailor-advanced")"
