# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qt_on_cocoa LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qt_on_cocoa Binary:
#####################################################################

qt_internal_add_manual_test(qt_on_cocoa
    GUI
    SOURCES
        main.mm
        rasterwindow.cpp rasterwindow.h
    LIBRARIES
        ${FWAppKit}
        Qt::Gui
        Qt::Quick
        Qt::Widgets
)
