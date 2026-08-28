# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qtouchevent LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qtouchevent Binary:
#####################################################################

qt_internal_add_manual_test(qtouchevent
    SOURCES
        form.ui
        main.cpp
        touchwidget.cpp touchwidget.h
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
