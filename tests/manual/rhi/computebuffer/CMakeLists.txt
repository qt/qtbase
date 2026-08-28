# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(computebuffer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## computebuffer Binary:
#####################################################################

qt_internal_add_manual_test(computebuffer
    GUI
    SOURCES
        computebuffer.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(computebuffer_resource_files
    "buffer.comp.qsb"
    "main.frag.qsb"
    "main.vert.qsb"
)

qt_internal_add_resource(computebuffer "computebuffer"
    PREFIX
        "/"
    FILES
        ${computebuffer_resource_files}
)


# TEMPLATE = "app"
