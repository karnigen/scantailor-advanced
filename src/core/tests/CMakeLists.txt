set(sources
    TestContentSpanFinder.cpp
    TestDeskewParams.cpp
    TestDeviationProvider.cpp
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
add_test(NAME settings_store_persistence_error_tests COMMAND core_tests "[settings-store-persistence-error]")

add_executable(settings_store_init_tests TestSettingsStoreInit.cpp)
target_link_libraries(settings_store_init_tests PRIVATE core Catch2::Catch2WithMain ${EXTRA_LIBS})
add_test(NAME settings_store_explicit_directory_tests COMMAND settings_store_init_tests "[settings-store-explicit]")
add_test(NAME settings_store_legacy_copy_tests COMMAND settings_store_init_tests "[settings-store-legacy]")
add_test(NAME settings_store_migration_tests COMMAND settings_store_init_tests "[settings-store-migration]")
add_test(NAME settings_store_empty_ini_migration_tests COMMAND settings_store_init_tests "[settings-store-empty-ini-migration]")
add_test(NAME settings_store_invalid_migration_candidate_tests COMMAND settings_store_init_tests "[settings-store-invalid-migration-candidate]")
add_test(NAME settings_store_fractional_integer_tests COMMAND settings_store_init_tests "[settings-store-fractional-integer]")
add_test(NAME settings_store_nonnumeric_value_tests COMMAND settings_store_init_tests "[settings-store-nonnumeric-value]")
add_test(NAME settings_store_nonfinite_number_migration_tests COMMAND settings_store_init_tests "[settings-store-nonfinite-number]")
add_test(NAME settings_store_invalid_size_type_tests COMMAND settings_store_init_tests "[settings-store-invalid-size-type]")
add_test(NAME settings_store_nonpositive_sizef_migration_tests COMMAND settings_store_init_tests "[settings-store-nonpositive-sizef]")
add_test(NAME settings_store_nonfinite_sizef_migration_tests COMMAND settings_store_init_tests "[settings-store-nonfinite-sizef-migration]")
add_test(NAME settings_store_nonfinite_double_tests COMMAND settings_store_init_tests "[settings-store-nonfinite-double]")
add_test(NAME settings_store_nonfinite_sizef_tests COMMAND settings_store_init_tests "[settings-store-nonfinite-sizef]")
add_test(NAME settings_store_nonpositive_size_tests COMMAND settings_store_init_tests "[settings-store-nonpositive-size]")
add_test(NAME settings_store_nonpositive_sizef_toml_tests COMMAND settings_store_init_tests "[settings-store-nonpositive-sizef-toml]")
add_test(NAME settings_store_migration_backup_tests COMMAND settings_store_init_tests "[settings-store-migration-backup]")
add_test(NAME settings_store_migration_race_tests COMMAND settings_store_init_tests "[settings-store-migration-race]")
add_test(NAME settings_store_concurrent_write_tests COMMAND settings_store_init_tests "[settings-store-concurrent-write]")
add_test(NAME settings_store_toml_precedence_tests COMMAND settings_store_init_tests "[settings-store-precedence]")
add_test(NAME settings_store_invalid_toml_tests COMMAND settings_store_init_tests "[settings-store-invalid-toml]")
add_test(NAME settings_store_invalid_type_tests COMMAND settings_store_init_tests "[settings-store-invalid-type]")
add_test(NAME settings_store_schema_type_tests COMMAND settings_store_init_tests "[settings-store-schema-type]")
add_test(NAME settings_store_numeric_type_tests COMMAND settings_store_init_tests "[settings-store-numeric-type]")
add_test(NAME settings_store_sizef_type_tests COMMAND settings_store_init_tests "[settings-store-sizef-type]")
add_test(NAME settings_store_geometry_fallback_tests COMMAND settings_store_init_tests "[settings-store-geometry-fallback]")
add_test(NAME settings_store_readonly_tests COMMAND settings_store_init_tests "[settings-store-readonly]")
add_test(NAME settings_store_invalid_ini_tests COMMAND settings_store_init_tests "[settings-store-invalid-ini]")
set_tests_properties(
    settings_store_migration_race_tests settings_store_concurrent_write_tests
    PROPERTIES ENVIRONMENT "SETTINGS_STORE_TEST_EXECUTABLE=$<TARGET_FILE:settings_store_init_tests>")
