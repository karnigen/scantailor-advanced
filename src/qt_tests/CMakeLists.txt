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
    "${CMAKE_BINARY_DIR}/src/core/filters/select_content/select_content_autogen/include"
    "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")

add_test(NAME qt_tests COMMAND qt_tests)
set_tests_properties(qt_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(deskew_qt_tests tst_deskew_qt.cpp)
set_target_properties(deskew_qt_tests PROPERTIES AUTOUIC ON AUTORCC ON)
target_link_libraries(
    deskew_qt_tests
    PRIVATE deskew core foundation ${Qt_Test_lib} ${Qt_Core_lib} ${Qt_Gui_lib} ${Qt_Widgets_lib} ${EXTRA_LIBS})
target_include_directories(
    deskew_qt_tests
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src/core/filters/deskew/deskew_autogen/include")

add_test(NAME deskew_qt_tests COMMAND deskew_qt_tests)
set_tests_properties(deskew_qt_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
