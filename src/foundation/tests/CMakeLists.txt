set(sources
    TestConstants.cpp
    TestFastStorage.cpp
    TestLineIntersectionScalar.cpp
    TestProximity.cpp
    TestUtils.cpp)

add_executable(foundation_tests ${sources})
target_link_libraries(
    foundation_tests
    PRIVATE foundation Catch2::Catch2WithMain ${EXTRA_LIBS})

add_test(NAME foundation_tests COMMAND foundation_tests)
