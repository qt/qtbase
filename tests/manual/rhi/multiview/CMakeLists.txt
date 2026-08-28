# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(multiview LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(multiview
    GUI
    SOURCES
        multiview.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_add_resource(multiview "multiview"
    PREFIX
        "/"
    FILES
        "multiview.vert.qsb"
        "multiview.frag.qsb"
        "texture.vert.qsb"
        "texture.frag.qsb"
)
