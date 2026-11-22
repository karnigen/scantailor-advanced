# Testing Guide (ScanTailor Advanced)

## Running unit tests

With the project configured with `BUILD_TESTS=ON` (the default):

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
ctest --output-on-failure
```

Or run each suite individually:

```bash
./core_tests --log_level=message
./foundation_tests --log_level=message
./math_tests --log_level=message
./imageproc_tests --log_level=message
./qt_tests
```

## Module coverage

### core_tests
- **ContentSpanFinder**: content span detection.
- **DeskewParams**: deskew filter parameters.
- **ImageId** / **PageId**: image and page identification, ordering, and subpages.
- **ImageAllocationLimit**: decoded-image memory estimates, rejection of oversized PNG/TIFF images before decoding, and successful TIFF loading after the limit is raised.
- **Margins**: margins and XML serialization.
- **ProjectMigration**: format-3 compatibility, migration backups, and format-4 serialization.
- **PageRange**: page ranges and alternating selection.
- **SelectContentApply**: applying the content filter.
- **SmartFilenameOrdering**: natural filename ordering.
- **Units**: unit conversion and formatting (px, mm, cm, in).

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

The UX tests use `QApplication` and can simulate interaction (clicks and keyboard input) without opening windows. They provide a foundation for adding tests for more widgets and dialogs.

## UI/UX tests and user workflows

The existing `qt_tests` suite already covers widget-level interactions, including simulated mouse clicks, keyboard input, and state/signal checks. Extend that suite for additional UX coverage rather than creating a duplicate test executable. Complete end-to-end user workflows are not currently covered. To improve UX and workflow coverage, consider:

1. **Extend Qt Test coverage**: Add tests to the existing `qt_tests` suite for additional widgets and dialogs.
2. **Integration tests**: Add tests or scripts that:
   - Open sample projects.
   - Run filters and check results on disk or in memory.

3. **End-to-end tests**: Use tools such as Squish or scripts that control the graphical application. These tests are more costly to maintain.

To measure code coverage with gcov/lcov (for supported compilers):

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DCMAKE_CXX_FLAGS="--coverage"
cmake --build .
ctest
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## Disabling tests

If the Boost `unit_test_framework` component is unavailable, or you do not need to build the test executables:

```bash
cmake .. -DBUILD_TESTS=OFF
```

This removes the requirement for Boost's `unit_test_framework` component only; the application still requires Boost headers and Qt.
