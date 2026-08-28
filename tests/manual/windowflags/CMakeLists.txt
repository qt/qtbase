# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_manual_windowflags LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_manual_windowflags Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_windowflags
    GUI
    SOURCES
        controllerwindow.cpp controllerwindow.h
        controls.cpp controls.h
        main.cpp
        previewwindow.cpp previewwindow.h
    NO_PCH_SOURCES
        controls.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
