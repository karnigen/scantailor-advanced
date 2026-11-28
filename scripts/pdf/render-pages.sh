#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/pdf/render-pages.sh [--dpi DPI] [--first-page N] [--last-page N] INPUT.pdf [OUTPUT_DIR]

Render complete PDF pages as LZW-compressed TIFF images using Poppler
pdftocairo. DPI defaults to 300 and OUTPUT_DIR defaults to the PDF folder.
Existing output files are never overwritten.

Set PDFTOCAIRO to use a specific pdftocairo executable.
EOF
}

fail() {
  echo "Error: $*" >&2
  exit 1
}

DPI=300
FIRST_PAGE=""
LAST_PAGE=""
while (($#)); do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --dpi)
      (($# >= 2)) || fail "--dpi requires a value."
      DPI="$2"
      shift 2
      ;;
    --first-page)
      (($# >= 2)) || fail "--first-page requires a value."
      FIRST_PAGE="$2"
      shift 2
      ;;
    --last-page)
      (($# >= 2)) || fail "--last-page requires a value."
      LAST_PAGE="$2"
      shift 2
      ;;
    --)
      shift
      break
      ;;
    -*)
      usage >&2
      fail "Unknown option: $1"
      ;;
    *)
      break
      ;;
  esac
done

if (($# == 0)) || (($# > 2)); then
  usage >&2
  exit 2
fi

if [[ ! "$DPI" =~ ^[0-9]{1,4}$ ]] || ((10#$DPI < 1 || 10#$DPI > 2400)); then
  fail "DPI must be an integer from 1 to 2400."
fi
for page_number in "$FIRST_PAGE" "$LAST_PAGE"; do
  if [[ -n "$page_number" ]] && { [[ ! "$page_number" =~ ^[0-9]{1,5}$ ]] || ((10#$page_number < 1)); }; then
    fail "Page numbers must be positive integers up to 99999."
  fi
done
if [[ -n "$FIRST_PAGE" && -n "$LAST_PAGE" ]] && ((10#$FIRST_PAGE > 10#$LAST_PAGE)); then
  fail "--first-page must not be greater than --last-page."
fi

PDF_INPUT="$1"
if [[ ! -f "$PDF_INPUT" ]]; then
  fail "PDF file does not exist: $PDF_INPUT"
fi
PDF_DIR="$(cd "$(dirname "$PDF_INPUT")" && pwd -P)"
PDF_PATH="$PDF_DIR/$(basename "$PDF_INPUT")"
PDF_NAME="$(basename "$PDF_PATH")"
FILE_STEM="${PDF_NAME%.*}"
OUTPUT_DIR="${2:-$PDF_DIR}"
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR="$(cd "$OUTPUT_DIR" && pwd -P)"

PDFTOCAIRO_BIN="${PDFTOCAIRO:-pdftocairo}"
if ! command -v "$PDFTOCAIRO_BIN" >/dev/null 2>&1; then
  fail "Poppler pdftocairo was not found. Install Poppler or set PDFTOCAIRO to its executable path."
fi

TEMP_DIR="$(mktemp -d "$OUTPUT_DIR/.${FILE_STEM}.render.XXXXXX")"
MOVED_FILES=()
COMMITTED=0
CHILD_PID=""
run_external() {
  "$@" &
  CHILD_PID=$!
  local exit_code=0
  wait "$CHILD_PID" || exit_code=$?
  CHILD_PID=""
  return "$exit_code"
}
cleanup() {
  if [[ -n "$CHILD_PID" ]]; then
    kill "$CHILD_PID" 2>/dev/null || true
    wait "$CHILD_PID" 2>/dev/null || true
    CHILD_PID=""
  fi
  local moved_file
  if [[ "$COMMITTED" != "1" ]]; then
    for moved_file in "${MOVED_FILES[@]-}"; do
      [[ -n "$moved_file" ]] || continue
      rm -f -- "$moved_file"
    done
  fi
  rm -rf -- "$TEMP_DIR"
}
trap cleanup EXIT
trap 'exit 130' INT TERM

PDFTOCAIRO_ARGS=(-tiff -tiffcompression lzw -r "$DPI")
if [[ -n "$FIRST_PAGE" ]]; then
  PDFTOCAIRO_ARGS+=(-f "$FIRST_PAGE")
fi
if [[ -n "$LAST_PAGE" ]]; then
  PDFTOCAIRO_ARGS+=(-l "$LAST_PAGE")
fi
run_external "$PDFTOCAIRO_BIN" "${PDFTOCAIRO_ARGS[@]}" "$PDF_PATH" "$TEMP_DIR/$FILE_STEM"

shopt -s nullglob
IMAGE_FILES=("$TEMP_DIR/$FILE_STEM"-*.tif "$TEMP_DIR/$FILE_STEM"-*.tiff)
if ((${#IMAGE_FILES[@]} == 0)); then
  fail "Poppler did not produce any TIFF pages."
fi

for image_file in "${IMAGE_FILES[@]}"; do
  image_dest="$OUTPUT_DIR/$(basename "$image_file")"
  if [[ -e "$image_dest" ]]; then
    fail "Refusing to overwrite existing file: $image_dest"
  fi
done

for image_file in "${IMAGE_FILES[@]}"; do
  image_dest="$OUTPUT_DIR/$(basename "$image_file")"
  mv -n "$image_file" "$image_dest"
  if [[ -e "$image_file" ]]; then
    fail "Could not move rendered page without overwriting: $image_dest"
  fi
  MOVED_FILES+=("$image_dest")
done
COMMITTED=1

printf 'Rendered %d page(s) at %s DPI to %s\n' "${#IMAGE_FILES[@]}" "$DPI" "$OUTPUT_DIR"
