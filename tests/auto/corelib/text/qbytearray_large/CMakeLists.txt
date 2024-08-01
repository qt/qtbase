# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qbytearray_large LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qbytearray_large
    SOURCES
        tst_qbytearray_large.cpp
    LIBRARIES
        Qt::Core
    TESTDATA "rfc3252.txt"
)

qt_internal_is_in_test_batch(in_batch tst_qbytearray_large)
if(in_batch)
    _qt_internal_test_batch_target_name(test_target)
else()
    set(test_target tst_qbytearray_large)
endif()

qt_internal_set_exceptions_flags(${test_target} TRUE)
