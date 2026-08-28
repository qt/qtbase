# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(computebuffer_indirect LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## computebuffer_indirect Binary:
#####################################################################

qt_internal_add_manual_test(computebuffer_indirect
    GUI
    SOURCES
        computebuffer_indirect.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(computebuffer_indirect_resource_files
    "dispatch_args.comp.qsb"
    "dispatch_consume.comp.qsb"
    "dispatch_reset.comp.qsb"
    "main.vert.qsb"
    "main.frag.qsb"
)

qt_internal_add_resource(computebuffer_indirect "computebuffer_indirect"
    PREFIX
        "/"
    FILES
        ${computebuffer_indirect_resource_files}
)
