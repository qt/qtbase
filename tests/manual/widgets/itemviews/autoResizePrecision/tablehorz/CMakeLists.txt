# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(testtable1 LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## testtable1 Binary:
#####################################################################

qt_internal_add_manual_test(testtable1
    GUI
    SOURCES
        testtable1.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
