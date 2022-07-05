# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qopenglwidget.pro.

#####################################################################
## tst_qopenglwidget Test:
#####################################################################

qt_internal_add_test(tst_qopenglwidget
    LOWDPI # special case
    SOURCES
        tst_qopenglwidget.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::OpenGLPrivate
        Qt::OpenGLWidgets
        Qt::Widgets
)
