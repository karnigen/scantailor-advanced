# Testing Guide (ScanTailor Advanced)

## Running unit tests

Use the repository build script for development builds and tests:

```bash
./build-vcpkg.sh
./build-vcpkg.sh --tests
```

Both commands use a preinstalled Qt SDK selected with `--qt-root` or the
`QT_ROOT` environment variable. The selected path is saved in the ignored
`.build-vcpkg-config` file for later runs. Qt is not a vcpkg dependency and is
never built by this script. Qt Testlib comes from the selected SDK regardless
of `--tests`; the option only enables the test targets and runs them through
CTest after the build. Other vcpkg dependencies use Release libraries.
The test targets cover typed settings round-trips, relative and invalid
configuration directories, legacy INI copying, profile placement, and the
configuration dialog.

CI workflows build/package the application with unit tests disabled. The
macOS workflow also runs a packaged-application smoke test; it does not run the
unit-test suites.

## Module coverage

### core_tests
- **ContentSpanFinder**: content span detection.
- **DeskewParams**: deskew filter parameters.
- **ImageId** / **PageId**: image and page identification, ordering, and subpages.
- **ImageAllocationLimit**: decoded-image memory estimates, unknown dimensions, allocation-limit caps and non-blocking updates, rejection of oversized PNG/TIFF images before decoding, and successful TIFF loading after the limit is raised.
- **Margins**: margins and XML serialization.
- **ProjectMigration**: format-3 compatibility, migration backups, and format-4 serialization.
- **PageRange**: page ranges and alternating selection.
- **SelectContentApply**: applying the content filter.
- **SmartFilenameOrdering**: natural filename ordering.
- **Units**: unit conversion and formatting (px, mm, cm, in).
- **SettingsStore**: typed value round-trips and compatibility with the legacy
  recent-project array layout.

### settings_store_explicit_directory_tests and settings_store_legacy_copy_tests
- Explicit relative configuration directory validation and creation.
- `config.toml` and `profiles/` placement.
- Copying existing standard INI settings, migrating them to TOML, and retaining the original as `config.ini.bak`.
- INI-to-TOML migration, inline-table formatting for collapsible-group states, TOML precedence, and malformed TOML handling.

### foundation_tests
- **Constants**: mathematical constants (PI, DPI/DPM and degree/radian conversions).
- **LineIntersectionScalar**: scalar line intersections.
- **Proximity**: distance between points and point-to-segment distance.
- **Utils**: locale-independent number-to-string conversion.

### math_tests
- **Hessians** (adiff): automatic differentiation.
- **LineBoundedByRect**: clipping a line to a rectangle.
- **LinearFunction** / **QuadraticFunction**: linear and quadratic functions.
- **MatrixCalc**: matrix operations.
- **SidesOfLine**: determining which side of a line points lie on.
- **SqDistApproximant**: squared-distance approximation.

### imageproc_tests
- **BinaryImage**, **Binarize**, **Morphology**, **RasterOp**, **Scale**, **Shear**, **Transform**, and others.
- **Dpi**: resolution (DPI) and XML serialization.

### qt_tests (Qt Test)
- **Logic tests (TestCoreQt)**: Units, `foundation::Utils`, `SmartFilenameOrdering`, and `QSignalSpy` (signals and arguments).
- **UX tests (TestUxQt)**:
  - **testButtonClickEmitsSignal**: clicking a `QPushButton` emits `clicked()` (simulated with `QTest::mouseClick`).
  - **testCheckBoxToggleChangesState**: changing the checked state with `setChecked` and checking the `toggled` signal.
  - **testLineEditAcceptsInput**: text entry with `QTest::keyClicks` and checking the resulting text.
  - **testCollapsibleGroupBoxStateAndSignal**: the project's widget's collapsed/expanded state and `collapsedStateChanged` signal.
  - **testErrorWidgetCreation**: creating an `ErrorWidget` and checking its children without showing a window.
  - **testConfigurationDialogShowsPathsAndReloadsSettings**: displayed settings
    and profile paths, read-only contents, Copy path, and Reload behavior.
  - **testSingleColumnSettingUpdatesButtonState**: the single-column setting
    updates the toggle button state in both directions.
- **Main-window tests (TestMainWindowAsync)**: a simulated cancelled batch verifies
  that the event loop remains responsive, the latest selected stage is used,
  and saving is deferred until the worker finishes and content geometry is reconciled.

The Qt tests use the offscreen platform and can simulate interaction without opening windows.
They compile application GUI sources for the tests but do not build Qt itself.

## Grayscale scaling: behavior and test coverage

`scaleToGray` is used for more than thumbnails: it also feeds background
estimation, page-split detection, dewarping, and the picture-area mask used in
output processing. Changes to its pixels can therefore affect subsequent
thresholding, detected boundaries, and output. Downscaling output is unchanged
by this work. The only production change is the fix of the integer-upscaling
defect described below, which previously returned uninitialized rows or read
beyond the image when the horizontal and vertical factors differed.

The original `ScaleTest` compared a 100×100 `Indexed8` noise image scaled to
50×50 and 80×80 against `QImage::scaled(..., SmoothTransformation)`, requiring
every pixel to be within 1 gray level. It used the process-global `rand()`,
whose sequence depends on the platform and earlier calls. The comparison dates
back at least to the 2016 repository history. A 2017 change restored comments
that explicitly left Qt comparisons for upscaling and mixed scaling disabled
because their results differed. The history examined does not establish whether
matching Qt was a long-term requirement or whether the differences were intended
to be addressed later.

The comparison is meaningful **if Qt pixel compatibility is the requirement**,
but Qt's smooth scaling is not an independent, stable definition of area
averaging. In the inspected Qt 6.11.3 implementation, the chosen path depends
on the image format, dimensions, and scale factor. For the original 100×100
`Indexed8` inputs at these sizes, Qt uses a paint-based bilinear path; for
`RGB32` it uses `smoothScaled`. The grayscale implementation uses block or
area-weighted averaging for downscaling, with 1/32-pixel fixed-point coordinates
in the generic path. Rounding and the selected Qt path can change the result.
Neither method's quality for real scanned pages follows solely from this
synthetic comparison.

One reproducible Release build measurement on Linux with Qt 6.11.3 used
the deterministic 100×100 high-contrast image in `TestScale.cpp`:

| Target | Qt input | Maximum difference | Mean absolute difference | Pixels differing by more than 1 |
|---|---|---:|---:|---:|
| 50×50 | Indexed8 | 3 | 1.1148 | 613 / 2500 |
| 50×50 | RGB32 | 1 | 0.5148 | 0 / 2500 |
| 80×80 | Indexed8 | 30 | 5.3700 | 5111 / 6400 |
| 80×80 | RGB32 | 1 | 0.5072 | 0 / 6400 |

For these two ratios, `scaleToGray` matched a separately calculated exact
area-overlap reference at every pixel. The ratios have exact 1/32-pixel
boundaries; this result does not imply exact area averages for all sizes.
The normal suite now tests both those noise cases and small hand-calculated
integer and fractional downscales. This replaces the original Qt equality as a
test of the current area-averaging behavior, **not** as proof that Qt
compatibility is unnecessary or that every scaling mode is correct.

The original `Indexed8` comparison and ±1 threshold remain as an opt-in
diagnostic with deterministic input and difference statistics. It is excluded
from normal CTest runs and fails for `Indexed8` on the Qt 6.11.3 setup above.
Its outcome may differ with Qt version or platform. If Qt compatibility is
required, resolve the difference rather than treating this diagnostic failure
as a passing functional test.

The same diagnostic also compares against `RGB32` input, which in Qt 6.11.3
selects `smoothScaled` instead of the paint-based path used for `Indexed8`. On
this setup the maximum difference was 1 for both target sizes. That indicates
the original ±1 mismatch came largely from Qt's format-specific path, not from
a general disagreement between both algorithms. It does not prove matching for
other sizes or platforms, and it is not a reason to store application grayscale
images as `RGB32`.

**Fixed scaling defect (integer upscaling with unequal factors):** the
integer-upscaling loop advanced its row counter by the horizontal factor
instead of the vertical one, and advanced the source row once per output group
regardless of the vertical factor. Scaling 2×2 to 2×4 read and wrote beyond the
image; scaling 2×2 to 4×2 left the last row unwritten, exposing uninitialized
memory. Both were reproduced with AddressSanitizer against the pre-fix code.
`scaleUpIntGrayToGray` now advances by the vertical factor. A regression test
covers 2×4, 4×2, 4×6 and 6×4 and checks exact pixel replication; the fixed code
is clean under AddressSanitizer. Equal-factor upscaling and all downscaling
paths are unaffected.

**Fixed scaling defect (single-pixel source axis):** non-integer upscaling of
an image whose source width or height is 1 px (for example 1×2 → 3×3) drove
`calc32xRatio1()` to a negative ratio, so destination coordinates mapped to
`sx = -1` and the interpolation read before the start of the row. In Release
this is a silent out-of-bounds read; in Debug the neighbor assertions fired
first. For a 1-pixel axis there is no second source pixel to interpolate
towards, so `calc32xRatio1()` now returns zero and the neighbor indices are
clamped. The output is constant along that axis and interpolated along the
other one. Regression tests cover 1×2 → 3×3 and 2×1 → 3×3; the pre-fix code
produced non-uniform rows (20, 20, 19 instead of 20, 20, 20) and a
heap-buffer-overflow under AddressSanitizer without assertions.

**Format decision:** grayscale images stay in `Format_Indexed8` (one byte per
pixel). `scaleToGray` operates directly on this buffer and never calls Qt's
scaler, so the `Indexed8`/`RGB32` difference affects only comparisons that use
Qt as a reference. Converting application grayscale images to `RGB32` would
quadruple memory use and change every algorithm that consumes `GrayImage`
without improving `scaleToGray` itself. Only thumbnails choose between the two
scalers: already-grayscale `Indexed8` images use `scaleToGray`, other formats
use Qt's smooth scaler, so visually identical inputs in different formats can
produce different thumbnail pixels.

Real-scan output comparisons and coverage of arbitrary ratios, mixed scaling,
upscaling, edges, and unusual DPI values are still needed before deciding
whether Qt pixel compatibility is a requirement.

## UI/UX tests and user workflows

The existing `qt_tests` suite already covers widget-level interactions, including simulated mouse clicks, keyboard input, and state/signal checks. Extend that suite for additional UX coverage rather than creating a duplicate test executable. Complete end-to-end user workflows are not currently covered. To improve UX and workflow coverage, consider:

1. **Extend Qt Test coverage**: Add tests to the existing `qt_tests` suite for additional widgets and dialogs.
2. **Integration tests**: Add tests or scripts that:
   - Open sample projects.
   - Run filters and check results on disk or in memory.

3. **End-to-end tests**: Use tools such as Squish or scripts that control the graphical application. These tests are more costly to maintain.
4. **Settings smoke check**: Change the remembered image allocation limit in Settings; verify that 0 restores Qt's default and that saved changes take effect after restarting the application.

Code-coverage builds are not currently supported by `build-vcpkg.sh`.

## Disabling tests

To build the application without building or running unit tests:

```bash
./build-vcpkg.sh
```

This skips Catch2 and the test executables. Qt Testlib remains part of the
selected preinstalled Qt SDK, so enabling tests later does not change Qt
dependencies.
