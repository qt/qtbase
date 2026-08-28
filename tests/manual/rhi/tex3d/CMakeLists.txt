# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tex3d LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(tex3d
    GUI
    SOURCES
        tex3d.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(tex3d_resource_files
    "texture3d.vert.qsb"
    "texture3d.frag.qsb"
)

qt_internal_add_resource(tex3d "tex3d"
    PREFIX
        "/"
    FILES
        ${tex3d_resource_files}
)
