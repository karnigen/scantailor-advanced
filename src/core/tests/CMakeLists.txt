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
    TestSettingsStore.cpp
    TestPageLayoutSettings.cpp
    TestSelectContentApply.cpp
    TestSmartFilenameOrdering.cpp
    TestUnits.cpp)

add_executable(core_tests ${sources})
target_link_libraries(
    core_tests
    PRIVATE core Catch2::Catch2WithMain ${EXTRA_LIBS})

add_test(NAME core_tests COMMAND core_tests)

add_executable(settings_store_init_tests TestSettingsStoreInit.cpp)
target_link_libraries(settings_store_init_tests PRIVATE core Catch2::Catch2WithMain ${EXTRA_LIBS})
add_test(NAME settings_store_explicit_directory_tests COMMAND settings_store_init_tests "[settings-store-explicit]")
add_test(NAME settings_store_legacy_copy_tests COMMAND settings_store_init_tests "[settings-store-legacy]")
add_test(NAME settings_store_migration_tests COMMAND settings_store_init_tests "[settings-store-migration]")
add_test(NAME settings_store_toml_precedence_tests COMMAND settings_store_init_tests "[settings-store-precedence]")
add_test(NAME settings_store_invalid_toml_tests COMMAND settings_store_init_tests "[settings-store-invalid-toml]")
add_test(NAME settings_store_invalid_type_tests COMMAND settings_store_init_tests "[settings-store-invalid-type]")
