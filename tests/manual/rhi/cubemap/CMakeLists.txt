# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(cubemap LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## cubemap Binary:
#####################################################################

qt_internal_add_manual_test(cubemap
    GUI
    SOURCES
        cubemap.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(cubemap_resource_files
    "c.png"
    "cubemap.frag.qsb"
    "cubemap.vert.qsb"
)

qt_internal_add_resource(cubemap "cubemap"
    PREFIX
        "/"
    FILES
        ${cubemap_resource_files}
)
