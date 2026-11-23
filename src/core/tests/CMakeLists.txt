set(sources
    TestContentSpanFinder.cpp
    TestDeskewParams.cpp
    TestEditableZoneSet.cpp
    TestFileNameDisambiguator.cpp
    TestImageAllocationLimit.cpp
    TestObliqueFinder.cpp
    TestImageId.cpp
    TestInteractionHandler.cpp
    TestInteractionState.cpp
    TestMargins.cpp
    TestPageId.cpp
    TestPageRange.cpp
    TestPageSequence.cpp
    TestProjectMigration.cpp
    TestPageLayoutSettings.cpp
    TestSelectContentApply.cpp
    TestSmartFilenameOrdering.cpp
    TestUnits.cpp)

add_executable(core_tests ${sources})
target_link_libraries(
    core_tests
    PRIVATE core Catch2::Catch2WithMain ${EXTRA_LIBS})

add_test(NAME core_tests COMMAND core_tests)