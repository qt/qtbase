# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(drawindirectcount LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## drawindirectcount Binary:
#####################################################################

qt_internal_add_manual_test(drawindirectcount
    GUI
    SOURCES
        drawindirectcount.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(drawindirectcount_resource_files
    "quads.vert.qsb"
    "quads.frag.qsb"
    "count_writer.comp.qsb"
)

qt_internal_add_resource(drawindirectcount "drawindirectcount"
    PREFIX
        "/"
    FILES
        ${drawindirectcount_resource_files}
)
