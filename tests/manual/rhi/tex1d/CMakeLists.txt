# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tex1d LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(tex1d
    GUI
    SOURCES
        tex1d.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(tex1d_resource_files
    "texture1d.vert.qsb"
    "texture1d.frag.qsb"
)

qt_internal_add_resource(tex1d "tex1d"
    PREFIX
        "/"
    FILES
        ${tex1d_resource_files}
)
