# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
