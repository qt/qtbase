# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(windowchildgeometry LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## windowchildgeometry Binary:
#####################################################################

qt_internal_add_manual_test(windowchildgeometry
    GUI
    SOURCES
        ../windowflags/controls.cpp ../windowflags/controls.h
        controllerwidget.cpp controllerwidget.h
        main.cpp
    INCLUDE_DIRECTORIES
        ../windowflags
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
