# ScanTailor project XML keys: format 3 vs. format 4

## Scope

This document compares project-format XML used by ScanTailor releases. Format 3 is the format
used by `v1.1.1`; format 4 starts with `v1.2.0`. Both formats use XML under the `<project>` root,
not JSON.

The table is about serialized keys and compatibility behavior. It is not a complete XML schema.
Runtime validation and migration must be verified with real project files.

## Root and path keys

| Key | Format 3 | Format 4 | Meaning / compatibility |
|---|---|---|---|
| `<project version>` | `3` | `4` | Format number. The reader supports formats 3 and 4; saving a v3 project writes format 4. |
| `<project outputDirectory>` | Absolute path in older v3 files; may be relative in later v3 files | Relative path when possible, including `..` | Directory for generated output. The v4 writer stores it relative to the project file when the path is on the same filesystem/drive. |
| `<project layoutDirection>` | `LTR` or `RTL` | `LTR` or `RTL` | User-interface layout direction. |
| `<directory path>` | Absolute path in older v3 files | Relative path when possible, including `..` | Directory containing source files. The reader resolves v4-relative paths from the project-file directory. |

## Filter keys

| XML location / key | Format 3 | Format 4 | Meaning |
|---|---|---|---|
| `<fix-orientation><image-settings>...` | Rotations and settings | Still supported | Per-image orientation and fix-stage data. |
| `<fix-orientation><image><trim enabled left right top bottom>` | Not part of the format-3 writer | Optional in v4 | Inward crop from the source image edges, in source pixels. `enabled="1"` enables trimming. |
| `<deskew algoContentBased>` | Present in v1.1.1 source | Present | `1` selects content-based deskew behavior; `0` uses the alternative/top-edge behavior. |
| `<deskew><params angle>` | Present | Present | Deskew angle in degrees. |
| `<deskew><params mode>` | `auto` or `manual` | `auto` or `manual` | Deskew algorithm/parameter mode. |
| `<deskew><params oblique>` | Present in later v3 files; absent in pages saved before oblique existed | Present | Oblique/shear angle in degrees. When `oblique`, `oblique-mode`, and `autoOblique` are all absent, the reader keeps oblique correction off at angle 0. |
| `<deskew><params><dependencies algoContentBased>` | Not written | `1` or `0` when known | Records which deskew algorithm produced the automatic angles. Missing data is treated as stale so automatic results are recomputed. |
| `<deskew><params oblique-mode>` | Not written | `auto` or `manual` | Separate mode for automatic or manual oblique correction. Off is stored as `manual` plus `oblique-disabled="1"` so older readers do not enable shear. |
| `<deskew><params oblique-disabled>` | Not written | `1` only for Off | Distinguishes Off from Manual at 0°. The current reader checks this before `oblique-mode`. |
| `<deskew><params autoOblique>` | Legacy boolean in an intermediate v4 implementation | Reader compatibility fallback | Legacy `1`/`0` representation for automatic oblique correction. The current reader prefers `oblique-disabled`, then `oblique-mode`, then `autoOblique`; it falls back to `mode` only when an `oblique` value exists. |
| `<color-or-grayscale fillOutsidePageBox>` | Not written | `1` or `0`; default `0` | Fill areas outside the original page box instead of only regular margins/offcut areas. |
| `<color-or-grayscale wienerCoef>` | Already written by `v1.1.1` | Present | Wiener denoising coefficient. Valid range is approximately `0..1`; values outside the range are reset to `0`. |
| `<color-or-grayscale wienerWinSize>` | Already written by `v1.1.1` | Present | Wiener filter window size. Values below `3` are reset to the default `5`. |
| `<color-or-grayscale fillingColor>` | `background`, `white`, or `black` | Same | Requested filling color. |

## Common project keys

Both versions use the same project-level data model for:

- `<directories>` and `<directory>`
- `<files>` and `<file>`
- `<images>` and `<image>`
- `<pages>` and `<page>`
- `<file-name-disambiguation>`
- `<filters>`
- `<size>` and `<dpi>`
- `<source_file>`
- page and content geometry elements

Attribute order in XML is not significant. Differences in numeric geometry, modification time,
file size, or processing values may result from a separate project or a later processing state;
they do not by themselves prove a format change.

## Format-3 compatibility and saving

The reader accepts format 3 and reports the known format-4 defaults before opening it. Opening
does not modify the source file. Saving writes format 4 and preserves an existing v3 file first
as a unique `<project-file>.v3.bak` (or `.v3.1.bak`, etc. if that name already exists). Save As
to a new file leaves the original v3 file untouched.

The compatibility warning reports the actual missing trim, fill, and oblique-mode data found in
the project, plus path values that the writer will store relatively where possible. Known
defaults are:

- Missing `<trim>` data means `enabled=0`, `left=0`, `top=0`, `right=0`, and `bottom=0`.
- Missing `fillOutsidePageBox` means `0`.
- `oblique-disabled="1"` selects Off. Otherwise, a missing `oblique-mode` falls back to
  `autoOblique` (`1` = auto, `0` = manual). If `autoOblique` is also absent but `oblique` exists,
  the deskew `mode` is used (`manual` = manual; other or missing values = auto). If all three
  oblique attributes are absent, oblique correction is Off at angle 0, regardless of deskew mode.

Default-parameter profiles (`.stp`) also accept versions 3 through the current project version.
Newly written profiles use the current version.

## Is changing only `version="3"` to `version="4"` enough?

It is not necessary for this application: the reader accepts v3 directly and uses the
compatibility behavior above. Manually changing the version string remains an unreliable
workaround for other builds because it does not convert or validate any other project data.

Manual replacement can appear to work when:

- the rest of the XML structure is already compatible;
- missing v4 keys receive valid defaults;
- no v3-only value or representation requires conversion;
- source paths and the project location allow the relative/absolute path data to resolve.

Examples of compatibility behavior:

- Missing `<trim>` data means trimming is disabled.
- Missing `fillOutsidePageBox` means `0`.
- Missing `wienerCoef` reads as `0.0`.
- Missing `wienerWinSize` may be defaulted or validated to `5`.
- Missing `oblique-mode` can fall back to legacy `autoOblique`, or to the deskew `mode` when an
  `oblique` value exists. If `oblique`, `oblique-mode`, and `autoOblique` are all missing, the
  reader selects Off at angle 0. A present `oblique-disabled="1"` explicitly selects Off.

The practical result still depends on the old project and must be tested. Keep the original
backup until the converted project has been opened and verified.

## Adding a key

When documenting or implementing a new serialized key, cover all four surfaces:

1. The C++ field and valid/default value.
2. XML serialization in `toXml()` or `saveSettings()`.
3. XML loading in the constructor or `loadSettings()`.
4. Compatibility behavior when the key is absent.

Prefer backward-compatible defaults and fallback parsing. Do not infer that missing XML keys
have no effect: some missing keys change to automatic mode or enable/disable a feature.
