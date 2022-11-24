# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from stereographicsview.pro.

#####################################################################
## stereographicsview Binary:
#####################################################################

set(CMAKE_AUTOUIC ON)

qt_internal_add_manual_test(stereographicsview
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        mygraphicsview.h
        mygraphicsview.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::OpenGLWidgets
        Qt::Widgets
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:stereographicsview.pro:<TRUE>:
# TEMPLATE = "app"
