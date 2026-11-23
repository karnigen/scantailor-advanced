set(sources
    main.cpp
    TestContentSpanFinder.cpp
    TestDeskewParams.cpp
    TestEditableZoneSet.cpp
    TestFileNameDisambiguator.cpp
    TestImageAllocationLimit.cpp
    TestObliqueFinder.cpp
    TestImageId.cpp
    TestInteractionHandler.cpp
    TestMargins.cpp
    TestPageId.cpp
    TestPageRange.cpp
    TestPageSequence.cpp
    TestProjectMigration.cpp
    TestSelectContentApply.cpp
    TestSmartFilenameOrdering.cpp
    TestUnits.cpp)

add_executable(core_tests ${sources})
target_link_libraries(
    core_tests
    PRIVATE core Boost::unit_test_framework ${EXTRA_LIBS})

add_test(NAME core_tests COMMAND core_tests --log_level=message)