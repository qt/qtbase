# Copyright (C) 2026 Aurélien Brooke <aurelien@bahiasoft.fr>
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_bench_qresource LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_bench_qresource
    SOURCES
        tst_bench_qresource.cpp
    LIBRARIES
        Qt::Test
)

qt_add_binary_resources(tst_bench_qresource_resource "resource.qrc"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/resource.rcc"
    OPTIONS -binary
)
add_dependencies(tst_bench_qresource tst_bench_qresource_resource)
