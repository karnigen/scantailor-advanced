#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/rc-build.sh [build selection]

Create or reuse an RC tag for the current commit and its GitHub prerelease.
With build selections, dispatch only the requested builds.

Selections:
  --linux PACKAGE   Build deb, rpm, appimage, or all Linux packages.
  --windows         Build the Windows package.
  --macos           Build the macOS package.
  --flatpak         Build the Flatpak bundle.
  --all             Build all platforms and Linux packages.
  -h, --help        Show this help.

Run from a clean, committed checkout. GitHub CLI must be authenticated.
With no selections, create or reuse the RC tag and prerelease without a build.
EOF
}

fail() {
  echo "Error: $*" >&2
  exit 1
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

LINUX_PACKAGE="none"
WINDOWS="false"
MACOS="false"
FLATPAK="false"

while (($#)); do
  case "$1" in
    --linux)
      (($# >= 2)) || fail "--linux requires deb, rpm, appimage, or all."
      case "$2" in
        deb|rpm|appimage|all) LINUX_PACKAGE="$2" ;;
        *) fail "Unsupported Linux package: $2" ;;
      esac
      shift 2
      ;;
    --windows)
      WINDOWS="true"
      shift
      ;;
    --macos)
      MACOS="true"
      shift
      ;;
    --flatpak)
      FLATPAK="true"
      shift
      ;;
    --all)
      LINUX_PACKAGE="all"
      WINDOWS="true"
      MACOS="true"
      FLATPAK="true"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "Unknown option: $1"
      ;;
  esac
done

VERSION="$(sed -n 's/^#define VERSION "\([^"]*\)".*/\1/p' version.h.in)"
[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] ||
  fail "Expected a stable X.Y.Z version in version.h.in; found '$VERSION'."

if [[ -n "$(git status --porcelain)" ]]; then
  fail "The working tree must be clean; commit or stash changes before tagging."
fi

gh auth status >/dev/null ||
  fail "Authenticate GitHub CLI with 'gh auth login' before creating an RC."

HEAD="$(git rev-parse HEAD)"
git fetch --tags origin

STABLE_TAG="v$VERSION"
if git show-ref --verify --quiet "refs/tags/$STABLE_TAG"; then
  fail "$STABLE_TAG already exists; do not create another RC for a released version."
fi

VERSION_RE='^v[0-9]+\.[0-9]+\.[0-9]+-rc([1-9][0-9]*)$'
MAX_RC=0
SELECTED_TAG=""
SELECTED_RC=0
while IFS= read -r candidate; do
  [[ -n "$candidate" ]] || continue
  [[ "$candidate" =~ $VERSION_RE ]] || continue
  rc_number="${BASH_REMATCH[1]}"
  [[ "${candidate%-rc*}" == "$STABLE_TAG" ]] || continue
  if ((rc_number > MAX_RC)); then
    MAX_RC="$rc_number"
  fi
  if [[ "$(git rev-parse "${candidate}^{commit}")" == "$HEAD" ]] &&
     ((rc_number > SELECTED_RC)); then
    SELECTED_TAG="$candidate"
    SELECTED_RC="$rc_number"
  fi
done < <(git tag --list "v${VERSION}-rc*")

if [[ -n "$SELECTED_TAG" ]]; then
  TAG="$SELECTED_TAG"
  git push origin "refs/tags/$TAG"
  echo "Reusing $TAG for commit $HEAD."
else
  TAG="${STABLE_TAG}-rc$((MAX_RC + 1))"
  git push origin "$HEAD:refs/tags/$TAG"
  git fetch --no-tags origin "refs/tags/$TAG:refs/tags/$TAG"
  [[ "$(git rev-parse "${TAG}^{commit}")" == "$HEAD" ]] ||
    fail "New tag $TAG does not point to the current commit."
  echo "Created $TAG for commit $HEAD."
fi

RELEASE_TAG="$(gh release list --limit 1000 --json tagName \
  --jq "map(select(.tagName == \"$TAG\")) | .[0].tagName // empty")"
if [[ -z "$RELEASE_TAG" ]]; then
  gh release create "$TAG" --verify-tag --prerelease --title "$TAG" \
    --generate-notes
else
  IS_DRAFT="$(gh release view "$TAG" --json isDraft --jq '.isDraft')"
  IS_PRERELEASE="$(gh release view "$TAG" --json isPrerelease --jq '.isPrerelease')"
  [[ "$IS_DRAFT" == "false" && "$IS_PRERELEASE" == "true" ]] ||
    fail "Existing release $TAG is not a published prerelease."
fi

if [[ "$LINUX_PACKAGE" != "none" || "$WINDOWS" == "true" ||
      "$MACOS" == "true" || "$FLATPAK" == "true" ]]; then
  gh workflow run rc-build.yml --ref "$TAG" \
    --field "linux=$LINUX_PACKAGE" \
    --field "windows=$WINDOWS" \
    --field "macos=$MACOS" \
    --field "flatpak=$FLATPAK"
  printf 'Dispatched selected builds for %s.\n' "$TAG"
else
  printf 'Created or reused %s without starting a build.\n' "$TAG"
fi
