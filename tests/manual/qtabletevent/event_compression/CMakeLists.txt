# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(event_compression LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## event_compression Binary:
#####################################################################

qt_internal_add_manual_test(event_compression
    SOURCES
        main.cpp
        mousestatwidget.cpp mousestatwidget.h
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
