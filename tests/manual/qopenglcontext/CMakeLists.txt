# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qopenglcontext.pro.

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

#### Keys ignored in scope 1:.:.:qopenglcontext.pro:<TRUE>:
# TEMPLATE = "app"
