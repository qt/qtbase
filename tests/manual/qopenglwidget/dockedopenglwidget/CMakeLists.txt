# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(dockedopenglwidget LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(CMAKE_AUTOUIC ON)

qt_internal_add_manual_test(dockedopenglwidget
    GUI
    SOURCES
        main.cpp
        geometryengine.cpp geometryengine.h
        mainwidget.cpp mainwidget.h
        mainwindow.cpp mainwindow.h
        mainwindow.ui
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::OpenGLWidgets
        Qt::Widgets
        Qt::WidgetsPrivate
)

qt_add_resources(dockedopenglwidget "dockedopenglwidget"
    PREFIX
        "/"
    FILES
        vshader.glsl
        fshader.glsl
        cube.png
)
