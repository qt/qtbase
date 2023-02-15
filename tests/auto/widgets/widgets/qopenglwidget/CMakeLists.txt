# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qopenglwidget Test:
#####################################################################

qt_internal_add_test(tst_qopenglwidget
    LOWDPI
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
