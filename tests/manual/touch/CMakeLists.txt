# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(touch LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## touch Binary:
#####################################################################

qt_internal_add_manual_test(touch
    SOURCES
        gestures.cpp gestures.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
