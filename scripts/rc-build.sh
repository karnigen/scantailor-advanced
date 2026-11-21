#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/rc-build.sh [build selection] [-y|--yes]

Create or reuse an RC tag for the current commit and its GitHub prerelease.
Without build selections, show a menu when run in a terminal.
The tag and prerelease are always created or reused; build selections control
which builds are dispatched.

Selections:
  --linux PACKAGE   Build deb, rpm, appimage, or all Linux packages.
  --windows         Build the Windows package.
  --macos           Build the macOS package.
  --flatpak         Build the Flatpak bundle.
  --all             Build all platforms and Linux packages.
  -y, --yes         Print the plan and run without confirmation or menu.
  -h, --help        Show this help.

Run from a clean, committed checkout. GitHub CLI must be authenticated.
Without a terminal, pass build selections or use --yes.
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
YES="false"
SELECTIONS_PROVIDED="false"

while (($#)); do
  case "$1" in
    --linux)
      (($# >= 2)) || fail "--linux requires deb, rpm, appimage, or all."
      case "$2" in
        deb|rpm|appimage|all) LINUX_PACKAGE="$2" ;;
        *) fail "Unsupported Linux package: $2" ;;
      esac
      SELECTIONS_PROVIDED="true"
      shift 2
      ;;
    --windows)
      WINDOWS="true"
      SELECTIONS_PROVIDED="true"
      shift
      ;;
    --macos)
      MACOS="true"
      SELECTIONS_PROVIDED="true"
      shift
      ;;
    --flatpak)
      FLATPAK="true"
      SELECTIONS_PROVIDED="true"
      shift
      ;;
    --all)
      LINUX_PACKAGE="all"
      WINDOWS="true"
      MACOS="true"
      FLATPAK="true"
      SELECTIONS_PROVIDED="true"
      shift
      ;;
    -y|--yes)
      YES="true"
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

if [[ "$SELECTIONS_PROVIDED" == "false" && "$YES" == "false" ]]; then
  [[ -t 0 ]] || fail "Interactive menu requires a terminal; specify builds or use --yes."

  while true; do
    printf '\nSelect builds (tag and GitHub prerelease are always included):\n'
    printf '  1) Linux package: %s\n' "$LINUX_PACKAGE"
    printf '  2) Windows: %s\n' "$WINDOWS"
    printf '  3) macOS: %s\n' "$MACOS"
    printf '  4) Flatpak: %s\n' "$FLATPAK"
    printf '  5) All builds\n'
    printf '  6) Continue\n'
    printf '  0) Cancel\n'
    if ! IFS= read -r -p "Select an option: " MENU_CHOICE; then
      fail "Could not read menu selection."
    fi

    case "$MENU_CHOICE" in
      1)
        printf '  1) deb\n  2) rpm\n  3) appimage\n  4) all Linux packages\n  0) none\n'
        if ! IFS= read -r -p "Linux package: " LINUX_CHOICE; then
          fail "Could not read Linux package selection."
        fi
        case "$LINUX_CHOICE" in
          1) LINUX_PACKAGE="deb" ;;
          2) LINUX_PACKAGE="rpm" ;;
          3) LINUX_PACKAGE="appimage" ;;
          4) LINUX_PACKAGE="all" ;;
          0) LINUX_PACKAGE="none" ;;
          *) printf 'Invalid Linux package selection.\n' >&2 ;;
        esac
        ;;
      2)
        [[ "$WINDOWS" == "true" ]] && WINDOWS="false" || WINDOWS="true"
        ;;
      3)
        [[ "$MACOS" == "true" ]] && MACOS="false" || MACOS="true"
        ;;
      4)
        [[ "$FLATPAK" == "true" ]] && FLATPAK="false" || FLATPAK="true"
        ;;
      5)
        LINUX_PACKAGE="all"
        WINDOWS="true"
        MACOS="true"
        FLATPAK="true"
        ;;
      6)
        break
        ;;
      0)
        printf 'Cancelled; no changes were made.\n'
        exit 0
        ;;
      *)
        printf 'Invalid menu selection.\n' >&2
        ;;
    esac
  done
fi

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
  TAG_ACTION="reuse existing tag"
else
  TAG="${STABLE_TAG}-rc$((MAX_RC + 1))"
  TAG_ACTION="create new tag"
fi

RELEASE_TAG="$(gh release list --limit 1000 --json tagName \
  --jq "map(select(.tagName == \"$TAG\")) | .[0].tagName // empty")"
if [[ -z "$RELEASE_TAG" ]]; then
  RELEASE_ACTION="create published prerelease"
else
  IS_DRAFT="$(gh release view "$TAG" --json isDraft --jq '.isDraft')"
  IS_PRERELEASE="$(gh release view "$TAG" --json isPrerelease --jq '.isPrerelease')"
  [[ "$IS_DRAFT" == "false" && "$IS_PRERELEASE" == "true" ]] ||
    fail "Existing release $TAG is not a published prerelease."
  RELEASE_ACTION="reuse existing published prerelease"
fi

printf '\nPlan:\n'
printf '  Version: %s\n' "$VERSION"
printf '  Tag: %s %s for commit %s\n' "$TAG_ACTION" "$TAG" "$HEAD"
printf '  GitHub release: %s\n' "$RELEASE_ACTION"
BUILD_SUMMARY=()
[[ "$LINUX_PACKAGE" == "none" ]] || BUILD_SUMMARY+=("Linux ($LINUX_PACKAGE)")
[[ "$WINDOWS" == "true" ]] && BUILD_SUMMARY+=("Windows")
[[ "$MACOS" == "true" ]] && BUILD_SUMMARY+=("macOS")
[[ "$FLATPAK" == "true" ]] && BUILD_SUMMARY+=("Flatpak")
if ((${#BUILD_SUMMARY[@]})); then
  printf '  Builds to dispatch: %s\n' "$(IFS=', '; printf '%s' "${BUILD_SUMMARY[*]}")"
else
  printf '  Builds to dispatch: none (tag and prerelease only)\n'
fi

if [[ "$YES" == "true" ]]; then
  printf 'Confirmation skipped (--yes).\n'
else
  [[ -t 0 ]] || fail "Cannot ask for confirmation without a terminal; rerun with --yes."
  if ! IFS= read -r -p "Continue? [y/N] " CONFIRMATION; then
    fail "Could not read confirmation."
  fi
  case "$CONFIRMATION" in
    y|Y|yes|YES)
      ;;
    *)
      printf 'Cancelled; no remote changes were made.\n'
      exit 0
      ;;
  esac
fi

if [[ "$TAG_ACTION" == "reuse existing tag" ]]; then
  git push origin "refs/tags/$TAG"
  echo "Reusing $TAG for commit $HEAD."
else
  git push origin "$HEAD:refs/tags/$TAG"
  git fetch --no-tags origin "refs/tags/$TAG:refs/tags/$TAG"
  [[ "$(git rev-parse "${TAG}^{commit}")" == "$HEAD" ]] ||
    fail "New tag $TAG does not point to the current commit."
  echo "Created $TAG for commit $HEAD."
fi

if [[ "$RELEASE_ACTION" == "create published prerelease" ]]; then
  gh release create "$TAG" --verify-tag --prerelease --title "$TAG" \
    --generate-notes
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
