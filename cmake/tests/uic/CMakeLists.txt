cmake_minimum_required(VERSION 3.12.0)

project(UicTest
    VERSION 1.0.0
    DESCRIPTION "Uic test"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX
)

## Add some paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../;${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/extra-cmake-modules/find-modules;${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/kwin")

## Qt specific setup common for all modules:
include(QtSetup)

include(../test.cmake)

fake_moc_results()
add_qt_executable(test_executable
  SOURCES
    ../main.cpp
    window.ui
)

fake_moc_results()
extend_target(test_executable SOURCES dialog/dialog.ui)

test_source_file(test_executable "${CMAKE_CURRENT_BINARY_DIR}/ui_window.h")
test_source_file(test_executable "${CMAKE_CURRENT_BINARY_DIR}/dialog/ui_dialog.h")

test_include_directory(test_executable "${CMAKE_CURRENT_BINARY_DIR}/dialog")
