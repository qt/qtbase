# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(drawindirectdepth LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## drawindirectdepth Binary:
#####################################################################

qt_internal_add_manual_test(drawindirectdepth
    GUI
    SOURCES
        drawindirectdepth.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(drawindirectdepth_resource_files
    "quads.vert.qsb"
    "quads.frag.qsb"
)

qt_internal_add_resource(drawindirectdepth "drawindirectdepth"
    PREFIX
        "/"
    FILES
        ${drawindirectdepth_resource_files}
)
