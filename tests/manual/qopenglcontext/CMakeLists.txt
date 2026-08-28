# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qopenglcontext LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qopenglcontext Binary:
#####################################################################

qt_internal_add_manual_test(qopenglcontext
    GUI
    SOURCES
        main.cpp
        qopenglcontextwindow.cpp qopenglcontextwindow.h
    LIBRARIES
        Qt::EglSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
)
