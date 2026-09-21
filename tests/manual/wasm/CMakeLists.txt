# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This directory only aggregates tests, so it has no test of its own to set up
# Qt's private build infrastructure. Do it here, and tell the tests below that
# it is already done - otherwise each of them calls project() again, which skips
# the executable finalizers they need (embind, EXPORT_NAME) on wasm.
if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(wasm_manual_tests VERSION 6.0.0 LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
    set(QT_BUILD_STANDALONE_TESTS ON)
endif()

add_subdirectory(eventloop)
add_subdirectory(rasterwindow)
add_subdirectory(a11y)
if(QT_FEATURE_widgets)
add_subdirectory(cursors)
add_subdirectory(localfiles)
add_subdirectory(localfonts)
add_subdirectory(qstdweb)
add_subdirectory(clipboard)
add_subdirectory(windowmanagement)
add_subdirectory(foreignwindows)
endif()
