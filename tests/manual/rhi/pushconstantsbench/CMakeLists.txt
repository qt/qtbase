# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(pushconstantsbench LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(pushconstantsbench
    GUI
    SOURCES
        pushconstantsbench.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(pushconstantsbench_resource_files
    "ubo.vert.qsb"
    "campc.vert.qsb"
    "objpc.vert.qsb"
    "pc.vert.qsb"
    "material.frag.qsb"
)

qt_internal_add_resource(pushconstantsbench "pushconstantsbench"
    PREFIX
        "/"
    FILES
        ${pushconstantsbench_resource_files}
)

set(imgui_base ../shared/imgui)
set(imgui_target pushconstantsbench)
include(${imgui_base}/imgui.cmakeinc)
