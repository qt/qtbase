# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtimer_vs_qmetaobject Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qtimer_vs_qmetaobject LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(qtimer_vs_qmetaobject
    SOURCES
        tst_qtimer_vs_qmetaobject.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Test
)
