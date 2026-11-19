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
| `<project version>` | `3` | `4` | Format number. The reader rejects a value different from `PROJECT_VERSION`; it does not perform a general format migration. |
| `<project outputDirectory>` | Absolute path in older v3 files; may be relative in later v3 files | Relative path when possible | Directory for generated output. The v4 writer stores it relative to the project file. |
| `<project layoutDirection>` | `LTR` or `RTL` | `LTR` or `RTL` | User-interface layout direction. |
| `<directory path>` | Absolute path in older v3 files | Relative path such as `.` or `..` | Directory containing source files. The reader resolves v4-relative paths from the project-file directory. |

## Filter keys

| XML location / key | Format 3 | Format 4 | Meaning |
|---|---|---|---|
| `<fix-orientation><image-settings>...` | Rotations and settings | Still supported | Per-image orientation and fix-stage data. |
| `<fix-orientation><image><trim enabled left right top bottom>` | Not part of the format-3 writer | Optional in v4 | Inward crop from the source image edges, in source pixels. `enabled="1"` enables trimming. |
| `<deskew algoContentBased>` | Present in v1.1.1 source | Present | `1` selects content-based deskew behavior; `0` uses the alternative/top-edge behavior. |
| `<deskew><params angle>` | Present | Present | Deskew angle in degrees. |
| `<deskew><params mode>` | `auto` or `manual` | `auto` or `manual` | Deskew algorithm/parameter mode. |
| `<deskew><params oblique>` | Present | Present | Oblique/shear angle in degrees. |
| `<deskew><params oblique-mode>` | Not written | `auto` or `manual` | Separate mode for automatic or manual oblique correction. |
| `<deskew><params autoOblique>` | Legacy boolean in an intermediate v4 implementation | Reader compatibility fallback | Legacy `1`/`0` representation for automatic oblique correction. The current reader prefers `oblique-mode`, then `autoOblique`, then `mode`. |
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

## Is changing only `version="3"` to `version="4"` enough?

It may work for some projects, but it is not a supported or reliable migration.

The current reader is strict: it accepts only `PROJECT_VERSION == 4` and has no general
v3-to-v4 migration pass. Replacing `3` with `4` can appear to work when:

- the rest of the XML structure is already compatible;
- missing v4 keys receive valid defaults;
- no v3-only value or representation requires conversion;
- source paths and the project location allow the relative/absolute path data to resolve.

Examples of compatibility behavior:

- Missing `<trim>` data means trimming is disabled.
- Missing `fillOutsidePageBox` means `0`.
- Missing `wienerCoef` reads as `0.0`.
- Missing `wienerWinSize` may be defaulted or validated to `5`.
- Missing `oblique-mode` can fall back to legacy `autoOblique`, or then to the deskew `mode`.

The practical result depends on the old project and must be tested. A safer approach is to open
the original v3 project in a v3-compatible or compatibility-capable build, save or export it
through an application version that performs the same relative-path and setting conversions,
and then verify the v4 project. Renaming only the version string is a workaround, not a
confirmed migration algorithm.

## Adding a key

When documenting or implementing a new serialized key, cover all four surfaces:

1. The C++ field and valid/default value.
2. XML serialization in `toXml()` or `saveSettings()`.
3. XML loading in the constructor or `loadSettings()`.
4. Compatibility behavior when the key is absent.

Prefer backward-compatible defaults and fallback parsing. Do not infer that missing XML keys
have no effect: some missing keys change to automatic mode or enable/disable a feature.
