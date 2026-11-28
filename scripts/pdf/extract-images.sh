#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/pdf/extract-images.sh INPUT.pdf [OUTPUT_DIR]

Extract embedded image streams with Poppler pdfimages, preserving their
original image formats where possible. OUTPUT_DIR defaults to the PDF folder.
Existing output files are never overwritten.

Set PDFIMAGES to use a specific pdfimages executable.
EOF
}

fail() {
  echo "Error: $*" >&2
  exit 1
}

if (($# == 1)) && [[ "$1" == "-h" || "$1" == "--help" ]]; then
  usage
  exit 0
fi
if (($# == 0)) || (($# > 2)); then
  usage >&2
  exit 2
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

PDFIMAGES_BIN="${PDFIMAGES:-pdfimages}"
if ! command -v "$PDFIMAGES_BIN" >/dev/null 2>&1; then
  fail "Poppler pdfimages was not found. Install Poppler or set PDFIMAGES to its executable path."
fi

TEMP_DIR="$(mktemp -d "$OUTPUT_DIR/.${FILE_STEM}.extract.XXXXXX")"
MOVED_FILES=()
COMMITTED=0
cleanup() {
  local moved_file
  if [[ "$COMMITTED" != "1" ]]; then
    for moved_file in "${MOVED_FILES[@]}"; do
      rm -f -- "$moved_file"
    done
  fi
  rm -rf -- "$TEMP_DIR"
}
trap cleanup EXIT

INFO_FILE="$TEMP_DIR/image-info.txt"
"$PDFIMAGES_BIN" -list "$PDF_PATH" > "$INFO_FILE"
"$PDFIMAGES_BIN" -all "$PDF_PATH" "$TEMP_DIR/$FILE_STEM"

shopt -s nullglob
IMAGE_FILES=("$TEMP_DIR/$FILE_STEM"-*)
if ((${#IMAGE_FILES[@]} == 0)); then
  fail "No embedded images were found. Use render-pages.sh for vector or text-based PDF pages."
fi

INFO_DEST="$OUTPUT_DIR/$FILE_STEM-image-info.txt"
if [[ -e "$INFO_DEST" ]]; then
  fail "Refusing to overwrite existing file: $INFO_DEST"
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
    fail "Could not move extracted image without overwriting: $image_dest"
  fi
  MOVED_FILES+=("$image_dest")
done

mv -n "$INFO_FILE" "$INFO_DEST"
if [[ -e "$INFO_FILE" ]]; then
  fail "Could not save image information without overwriting: $INFO_DEST"
fi
MOVED_FILES+=("$INFO_DEST")
COMMITTED=1

printf 'Extracted %d embedded image(s) to %s\n' "${#IMAGE_FILES[@]}" "$OUTPUT_DIR"
printf 'Image dimensions and effective page DPI: %s\n' "$INFO_DEST"
