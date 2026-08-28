# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(hdr LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(hdr
    GUI
    SOURCES
        hdr.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_add_resource(hdr "hdr"
    PREFIX
        "/"
    FILES
        "hdrtexture.vert.qsb"
        "hdrtexture.frag.qsb"
)

set(imgui_base ../shared/imgui)
set(imgui_target hdr)
include(${imgui_base}/imgui.cmakeinc)
