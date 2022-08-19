# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(QtMakeOutputFileTest
    VERSION 1.0.0
    DESCRIPTION "qt_make_output_file test"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

## Add some paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../;${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/extra-cmake-modules/find-modules;${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/kwin")

include(QtBuild)

qt_make_output_file("foo.cpp" "" ".mapped" "/tmp/foo" "/tmp/bar" outfile)
assert(outfile STREQUAL "/tmp/bar/foo.mapped")

qt_make_output_file("../foo.cpp" "prefix_" ".cpp" "/tmp/foo" "/tmp/bar" outfile)
assert(outfile STREQUAL "/tmp/bar/__/prefix_foo.cpp")

qt_make_output_file("/tmp/bar/foo.cpp" "prefix_" ".cpp" "/tmp/foo" "/tmp/bar" outfile)
assert(outfile STREQUAL "/tmp/bar/prefix_foo.cpp")
