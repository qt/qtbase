# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## openglwidget Binary:
#####################################################################

qt_internal_add_manual_test(openglwidget
    GUI
    SOURCES
        main.cpp
        openglwidget.cpp openglwidget.h
    NO_PCH_SOURCES
        main.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::OpenGLWidgets
        Qt::Widgets
        Qt::WidgetsPrivate
)
