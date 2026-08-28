# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qglyphrun LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(qglyphrun
    GUI
    SOURCES
        main.cpp
        controller.h controller.cpp controller.ui
        view.h view.cpp
        glyphruninspector.h glyphruninspector.cpp
        singleglyphrun.h singleglyphrun.cpp singleglyphrun.ui
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
