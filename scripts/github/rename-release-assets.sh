#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/github/rename-release-assets.sh [-y|--yes] RELEASE_TAG SOURCE_VERSION TARGET_VERSION COLLISION_POLICY

Rename versioned assets on an existing GitHub Release. This helper is shared by
the RC build and stable release workflows:
  - RC builds add -rcN to asset names so downloaded files are clearly prereleases.
  - Stable promotion removes -rcN so final release assets use stable names.

The helper changes release asset names only; it does not change package contents
or the version embedded in a package. Keeping this operation in one helper lets
both workflows use the same filename and collision rules.
The repository is read from GITHUB_REPOSITORY in Actions, or detected with
GitHub CLI from the current checkout when run manually.

By default, print the plan and ask before changing the release. Use -y or --yes
to apply the plan without prompting (required when running without a terminal).

Collision policies:
  prefer-source  Keep SOURCE_VERSION assets if both source and target names exist.
  prefer-target  Keep TARGET_VERSION assets if both source and target names exist.

The asset types handled are Linux DEB, RPM, AppImage and Flatpak, plus macOS and
Windows archives. Only uploaded, non-empty assets are eligible to replace
another asset; if neither colliding asset is complete, the helper refuses to
change the release.

Examples:
  # Rename stable asset names to RC names, then confirm interactively.
  scripts/github/rename-release-assets.sh \
    v1.3.0-rc1 1.3.0 1.3.0-rc1 prefer-target

  # Restore stable names when promoting the RC; run without a prompt.
  scripts/github/rename-release-assets.sh --yes \
    v1.3.0-rc1 1.3.0-rc1 1.3.0 prefer-source
EOF
}

fail() {
  echo "Error: $*" >&2
  exit 1
}

asset_is_complete() {
  [[ "$1" == "uploaded" && "$2" =~ ^[1-9][0-9]*$ ]]
}

YES="false"
POSITIONAL=()
while (($#)); do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -y|--yes)
      YES="true"
      ;;
    *)
      POSITIONAL+=("$1")
      ;;
  esac
  shift
done

((${#POSITIONAL[@]} == 4)) || {
  usage >&2
  exit 2
}

RELEASE_TAG="${POSITIONAL[0]}"
SOURCE_VERSION="${POSITIONAL[1]}"
TARGET_VERSION="${POSITIONAL[2]}"
COLLISION_POLICY="${POSITIONAL[3]}"
VERSION_RE='^[0-9]+\.[0-9]+\.[0-9]+(-rc[1-9][0-9]*)?$'

[[ "$SOURCE_VERSION" =~ $VERSION_RE ]] || fail "Invalid source version: $SOURCE_VERSION"
[[ "$TARGET_VERSION" =~ $VERSION_RE ]] || fail "Invalid target version: $TARGET_VERSION"
[[ "$SOURCE_VERSION" != "$TARGET_VERSION" ]] || fail "Source and target versions must differ."
REPOSITORY="${GITHUB_REPOSITORY:-}"
if [[ -z "$REPOSITORY" ]]; then
  if ! REPOSITORY="$(gh repo view --json nameWithOwner --jq '.nameWithOwner')" ||
     [[ -z "$REPOSITORY" ]]; then
    fail "Could not determine the GitHub repository; run from its checkout with gh authenticated or set GITHUB_REPOSITORY."
  fi
fi
case "$COLLISION_POLICY" in
  prefer-source|prefer-target) ;;
  *) fail "Invalid collision policy: $COLLISION_POLICY" ;;
esac

SUFFIXES=(
  linux-x86_64.deb
  linux-x86_64.rpm
  linux-x86_64.AppImage
  linux-x86_64.flatpak
  macos-arm64.zip
  windows-x64.zip
)

ASSETS="$(gh api "repos/${REPOSITORY}/releases/tags/${RELEASE_TAG}" \
  --jq '.assets[] | [.id, .name, .state, .size] | @tsv')"
PLAN_ACTIONS=()
PLAN_SOURCE_ASSETS=()
PLAN_SOURCE_IDS=()
PLAN_TARGET_ASSETS=()
PLAN_REPLACE_TARGET=()

for suffix in "${SUFFIXES[@]}"; do
  SOURCE_ASSET="scantailor-advanced-${SOURCE_VERSION}-${suffix}"
  TARGET_ASSET="scantailor-advanced-${TARGET_VERSION}-${suffix}"
  SOURCE_ID=""
  SOURCE_STATE=""
  SOURCE_SIZE=""
  TARGET_ID=""
  TARGET_STATE=""
  TARGET_SIZE=""
  while IFS=$'\t' read -r asset_id asset_name asset_state asset_size; do
    [[ -n "${asset_name:-}" ]] || continue
    if [[ "$asset_name" == "$SOURCE_ASSET" ]]; then
      SOURCE_ID="$asset_id"
      SOURCE_STATE="$asset_state"
      SOURCE_SIZE="$asset_size"
    elif [[ "$asset_name" == "$TARGET_ASSET" ]]; then
      TARGET_ID="$asset_id"
      TARGET_STATE="$asset_state"
      TARGET_SIZE="$asset_size"
    fi
  done <<< "$ASSETS"
  [[ -n "$SOURCE_ID" ]] || continue

  SOURCE_COMPLETE="false"
  TARGET_COMPLETE="false"
  if asset_is_complete "$SOURCE_STATE" "$SOURCE_SIZE"; then
    SOURCE_COMPLETE="true"
  fi
  if asset_is_complete "$TARGET_STATE" "$TARGET_SIZE"; then
    TARGET_COMPLETE="true"
  fi

  PLAN_SOURCE_ASSETS+=("$SOURCE_ASSET")
  PLAN_SOURCE_IDS+=("$SOURCE_ID")
  PLAN_TARGET_ASSETS+=("$TARGET_ASSET")
  if [[ -n "$TARGET_ID" ]]; then
    if [[ "$SOURCE_COMPLETE" == "true" && "$TARGET_COMPLETE" == "true" ]]; then
      if [[ "$COLLISION_POLICY" == "prefer-target" ]]; then
        PLAN_ACTIONS+=("delete-source")
        PLAN_REPLACE_TARGET+=("false")
      else
        PLAN_ACTIONS+=("rename-source")
        PLAN_REPLACE_TARGET+=("true")
      fi
    elif [[ "$SOURCE_COMPLETE" == "true" ]]; then
      PLAN_ACTIONS+=("rename-source")
      PLAN_REPLACE_TARGET+=("true")
    elif [[ "$TARGET_COMPLETE" == "true" ]]; then
      PLAN_ACTIONS+=("delete-source")
      PLAN_REPLACE_TARGET+=("false")
    else
      fail "Both $SOURCE_ASSET and $TARGET_ASSET are incomplete; refusing to replace either."
    fi
  else
    [[ "$SOURCE_COMPLETE" == "true" ]] ||
      fail "Source asset $SOURCE_ASSET is not uploaded and non-empty; refusing to rename it."
    PLAN_ACTIONS+=("rename-source")
    PLAN_REPLACE_TARGET+=("false")
  fi
done

printf 'Release asset rename plan:\n'
printf '  Repository: %s\n' "$REPOSITORY"
printf '  Release: %s\n' "$RELEASE_TAG"
printf '  Version names: %s -> %s\n' "$SOURCE_VERSION" "$TARGET_VERSION"
printf '  Collision policy: %s\n' "$COLLISION_POLICY"
if ((${#PLAN_ACTIONS[@]} == 0)); then
  printf '  No matching assets need changes.\n'
  exit 0
fi

for i in "${!PLAN_ACTIONS[@]}"; do
  case "${PLAN_ACTIONS[$i]}" in
    delete-source)
      printf '  Keep complete %s; delete %s.\n' \
        "${PLAN_TARGET_ASSETS[$i]}" "${PLAN_SOURCE_ASSETS[$i]}"
      ;;
    rename-source)
      if [[ "${PLAN_REPLACE_TARGET[$i]}" == "true" ]]; then
        printf '  Replace %s with %s.\n' \
          "${PLAN_TARGET_ASSETS[$i]}" "${PLAN_SOURCE_ASSETS[$i]}"
      else
        printf '  Rename %s to %s.\n' \
          "${PLAN_SOURCE_ASSETS[$i]}" "${PLAN_TARGET_ASSETS[$i]}"
      fi
      ;;
  esac
done

if [[ "$YES" == "true" ]]; then
  printf 'Confirmation skipped (--yes).\n'
else
  [[ -t 0 ]] || fail "Cannot ask for confirmation without a terminal; rerun with --yes."
  if ! IFS= read -r -p "Apply these release asset changes? [y/N] " CONFIRMATION; then
    fail "Could not read confirmation."
  fi
  case "$CONFIRMATION" in
    y|Y|yes|YES)
      ;;
    *)
      printf 'Cancelled; no release assets were changed.\n'
      exit 0
      ;;
  esac
fi

for i in "${!PLAN_ACTIONS[@]}"; do
  SOURCE_ASSET="${PLAN_SOURCE_ASSETS[$i]}"
  SOURCE_ID="${PLAN_SOURCE_IDS[$i]}"
  TARGET_ASSET="${PLAN_TARGET_ASSETS[$i]}"
  if [[ "${PLAN_ACTIONS[$i]}" == "delete-source" ]]; then
    gh release delete-asset "$RELEASE_TAG" "$SOURCE_ASSET" \
      --repo "$REPOSITORY" --yes
    continue
  fi
  if [[ "${PLAN_REPLACE_TARGET[$i]}" == "true" ]]; then
    gh release delete-asset "$RELEASE_TAG" "$TARGET_ASSET" \
      --repo "$REPOSITORY" --yes
  fi
  [[ "$SOURCE_ID" =~ ^[0-9]+$ ]] || fail "Invalid asset ID for $SOURCE_ASSET."
  RENAMED_ASSET="$(gh api --method PATCH \
    "repos/${REPOSITORY}/releases/assets/${SOURCE_ID}" \
    -f "name=$TARGET_ASSET" --jq '.name')"
  [[ "$RENAMED_ASSET" == "$TARGET_ASSET" ]] ||
    fail "Failed to rename $SOURCE_ASSET to $TARGET_ASSET."
  printf 'Renamed %s to %s.\n' "$SOURCE_ASSET" "$TARGET_ASSET"
done

ASSETS="$(gh api "repos/${REPOSITORY}/releases/tags/${RELEASE_TAG}" \
  --jq '.assets[] | [.id, .name, .state, .size] | @tsv')"
for suffix in "${SUFFIXES[@]}"; do
  SOURCE_ASSET="scantailor-advanced-${SOURCE_VERSION}-${suffix}"
  SOURCE_REMAINS="false"
  while IFS=$'\t' read -r asset_id asset_name asset_state asset_size; do
    if [[ "$asset_name" == "$SOURCE_ASSET" ]]; then
      SOURCE_REMAINS="true"
    fi
  done <<< "$ASSETS"
  if [[ "$SOURCE_REMAINS" == "true" ]]; then
    fail "Old asset name remains after rename: $SOURCE_ASSET."
  fi
done

for i in "${!PLAN_TARGET_ASSETS[@]}"; do
  TARGET_ASSET="${PLAN_TARGET_ASSETS[$i]}"
  TARGET_COMPLETE="false"
  while IFS=$'\t' read -r asset_id asset_name asset_state asset_size; do
    if [[ "$asset_name" == "$TARGET_ASSET" ]] &&
       asset_is_complete "$asset_state" "$asset_size"; then
      TARGET_COMPLETE="true"
    fi
  done <<< "$ASSETS"
  [[ "$TARGET_COMPLETE" == "true" ]] ||
    fail "Target asset $TARGET_ASSET is not uploaded and non-empty."
done
