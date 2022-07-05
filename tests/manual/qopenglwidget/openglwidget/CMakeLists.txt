# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
