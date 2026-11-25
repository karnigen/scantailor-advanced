set(sources
    main.cpp
    tst_core_qt.cpp
    tst_mainwindow.cpp
    tst_ux_qt.cpp
    ${SCANTAILOR_QT_TEST_APP_SOURCES})

add_executable(qt_tests ${sources})
set_target_properties(qt_tests PROPERTIES AUTOUIC ON AUTORCC ON)
target_link_libraries(
    qt_tests
    PRIVATE core foundation ${Qt_Test_lib} ${Qt_Core_lib} ${Qt_Gui_lib} ${Qt_Widgets_lib} ${EXTRA_LIBS})
target_include_directories(
    qt_tests
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/app"
    "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")

add_test(NAME qt_tests COMMAND qt_tests)
set_tests_properties(qt_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
