# Copyright (C) 2022 The Qt Company Ltd.
# Copyright (C) 2026 Intel Corporation.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qatomicinteger LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(types
    char
    char16_t
    char32_t
    int
    long
    qlonglong
    qptrdiff
    quintptr
    qulonglong
    schar
    short
    uchar
    uint
    ulong
    ushort
    wchar_t
)
foreach(type ${types})
    qt_internal_add_test(tst_qatomicinteger_${type}
        SOURCES
            tst_qatomicinteger.cpp
        DEFINES
            QATOMIC_TEST_TYPE=${type}
            tst_QAtomicIntegerXX=tst_QAtomicInteger_${type}
    )
endforeach()
unset(types)
