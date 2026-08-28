# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_manual_instancing LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## instancing Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_instancing
    GUI
    SOURCES
        instancing.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(instancing_resource_files
    "inst.frag.qsb"
    "inst.vert.qsb"
)

qt_internal_add_resource(tst_manual_instancing "instancing"
    PREFIX
        "/"
    FILES
        ${instancing_resource_files}
)
