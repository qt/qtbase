# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(widget LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## widget Binary:
#####################################################################

qt_internal_add_manual_test(widget
    GUI
    SOURCES
        ../shared/shared.h
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
