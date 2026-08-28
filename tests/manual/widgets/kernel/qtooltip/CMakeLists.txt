# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qtooltip LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qtooltip Binary:
#####################################################################

qt_internal_add_manual_test(qtooltip
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
