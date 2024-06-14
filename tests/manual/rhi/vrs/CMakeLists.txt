# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(vrs LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(vrs
    GUI
    SOURCES
        vrs.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/qt256.png"
    PROPERTIES QT_RESOURCE_ALIAS "qt256.png"
)
qt_internal_add_resource(vrs "vrs"
    PREFIX
        "/"
    FILES
        "../shared/texture.vert.qsb"
        "../shared/texture.frag.qsb"
        "../shared/qt256.png"
)

qt_internal_extend_target(vrs CONDITION QT_FEATURE_metal
    SOURCES
        vrs_metaltest.mm
)

set(imgui_base ../shared/imgui)
set(imgui_target vrs)
include(${imgui_base}/imgui.cmakeinc)
