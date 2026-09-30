# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_QFontEngine Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_bench_QFontEngine LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_bench_QFontEngine
    SOURCES
        ../../../../auto/gui/text/qfontengine/tst_qfontengine.cpp
        tst_qfontengine.cpp
    DEFINES
        QFONTENGINE_BENCHMARK
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
)
