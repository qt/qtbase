# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(shadowmap LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## shadowmap Binary:
#####################################################################

qt_internal_add_manual_test(shadowmap
    GUI
    SOURCES
        shadowmap.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(shadowmap_resource_files
    "main.frag.qsb"
    "main.vert.qsb"
    "shadowmap.frag.qsb"
    "shadowmap.vert.qsb"
)

qt_internal_add_resource(shadowmap "shadowmap"
    PREFIX
        "/"
    FILES
        ${shadowmap_resource_files}
)
