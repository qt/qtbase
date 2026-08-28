# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(gtk-container LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_find_package(GTK3 MODULE)
qt_find_package(X11 MODULE)

#####################################################################
## gtk-container Binary:
#####################################################################

qt_internal_add_manual_test(gtk-container
    GUI
    SOURCES
        gtk-container.cpp
        DEFINES
        GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6
    LIBRARIES
        PkgConfig::GTK3
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        X11::X11
)
