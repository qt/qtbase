# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(pushconstants LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(pushconstants
    GUI
    SOURCES
        pushconstants.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(pushconstants_resource_files
    "material.vert.qsb"
    "material.frag.qsb"
)

qt_internal_add_resource(pushconstants "pushconstants"
    PREFIX
        "/"
    FILES
        ${pushconstants_resource_files}
)
