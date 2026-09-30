# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_bench_qlibraryinfo LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_bench_qlibraryinfo
    SOURCES
        tst_bench_qlibraryinfo.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)

qt_internal_add_resource(tst_bench_qlibraryinfo "qtconffiles"
    PREFIX "/"
    FILES
        absolute.qt.conf
        relative.qt.conf
        platforms.qt.conf
)
