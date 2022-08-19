# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from openglwidget.pro.

#####################################################################
## openglwidget Binary:
#####################################################################

qt_internal_add_manual_test(openglwidget
    GUI
    SOURCES
        main.cpp
        openglwidget.cpp openglwidget.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::OpenGLWidgets
        Qt::Widgets
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:openglwidget.pro:<TRUE>:
# TEMPLATE = "app"
