# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(displacement LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(displacement
    GUI
    SOURCES
        displacement.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_add_resource(displacement "displacement"
    PREFIX
        "/"
    FILES
        "material.vert.qsb"
        "material.tesc.qsb"
        "material.tese.qsb"
        "material.frag.qsb"
        "heightmap.png"
)

set(imgui_base ../shared/imgui)
set(imgui_target displacement)
include(${imgui_base}/imgui.cmakeinc)
