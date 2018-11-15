cmake_minimum_required(VERSION 3.12.0)

project(QrcTest
    VERSION 1.0.0
    DESCRIPTION "Qrc test"
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
    qrc.qrc
)

fake_moc_results()
extend_target(test_executable SOURCES dialog/dialog.qrc)

test_source_file(test_executable "${CMAKE_CURRENT_BINARY_DIR}/qrc_qrc.cpp" BUILD)
test_source_file(test_executable "${CMAKE_CURRENT_BINARY_DIR}/dialog/qrc_dialog.cpp" BUILD)

test_include_directory(test_executable "${CMAKE_CURRENT_BINARY_DIR}/dialog" UNKNOWN)
