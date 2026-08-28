# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(noninstanced LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## noninstanced Binary:
#####################################################################

qt_internal_add_manual_test(noninstanced
    GUI
    SOURCES
        noninstanced.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(noninstanced_resource_files
    "material.frag.qsb"
    "material.vert.qsb"
)

qt_internal_add_resource(noninstanced "noninstanced"
    PREFIX
        "/"
    FILES
        ${noninstanced_resource_files}
)
