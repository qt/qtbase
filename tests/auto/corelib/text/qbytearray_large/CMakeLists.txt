# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qbytearray_large LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(compile_and_link_options "")
if(WASM)
    list(APPEND compile_and_link_options "-fexceptions")
endif()

qt_internal_add_test(tst_qbytearray_large
    SOURCES
        tst_qbytearray_large.cpp
    LIBRARIES
        Qt::Core
    TESTDATA "rfc3252.txt"
    COMPILE_OPTIONS ${compile_and_link_options}
    LINK_OPTIONS ${compile_and_link_options}
)

