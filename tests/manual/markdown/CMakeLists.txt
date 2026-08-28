# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(html2md LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## html2md Binary:
#####################################################################

qt_internal_add_manual_test(html2md
    GUI
    SOURCES
        html2md.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
)
