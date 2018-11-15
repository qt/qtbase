cmake_minimum_required(VERSION 3.12.0)

project(MocTest
    VERSION 1.0.0
    DESCRIPTION "Moc test"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX
)

## Add some paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../;${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/extra-cmake-modules/find-modules;${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/kwin")

## Qt specific setup common for all modules:
include(QtSetup)

include(../test.cmake)

fake_moc_results(MOC foo.cpp sub2/foobar.h MOC_AND_BUILD main.cpp sub/bar.h)
add_qt_executable(test_executable
  SOURCES
    ../main.cpp
    sub/bar.h
    sub2/foobar.h
    foo.cpp
)

test_source_file(test_executable "${CMAKE_CURRENT_BINARY_DIR}/main.moc" BUILD)
test_source_file(test_executable "${CMAKE_CURRENT_BINARY_DIR}/sub/moc_bar.cpp" BUILD)
test_source_file(test_executable "${CMAKE_CURRENT_BINARY_DIR}/sub2/moc_foobar.cpp")
test_source_file(test_executable "${CMAKE_CURRENT_BINARY_DIR}/foo.moc")

test_include_directory(test_executable "${CMAKE_CURRENT_BINARY_DIR}/sub" UNKNOWN)
test_include_directory(test_executable "${CMAKE_CURRENT_BINARY_DIR}/sub2")

