# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(polygonmode LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(polygonmode
    GUI
    SOURCES
        polygonmode.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(polygonmode_resource_files
    "test.vert.qsb"
    "test.frag.qsb"
)

qt_internal_add_resource(polygonmode "polygonmode"
    PREFIX
        "/"
    FILES
        ${polygonmode_resource_files}
)
