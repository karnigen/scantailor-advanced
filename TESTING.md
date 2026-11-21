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
- **Margins**: margins and XML serialization.
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

There are currently no automated interface or end-to-end workflow tests. To improve UX and workflow coverage, consider:

1. **Qt Test**: Add a test executable that uses `QTest` to:
   - Create widgets and dialogs without showing a window.
   - Simulate events (clicks and keyboard input) and check state.

2. **Integration tests**: Add scripts or tests that:
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

If Boost or test dependencies are unavailable:

```bash
cmake .. -DBUILD_TESTS=OFF
```
