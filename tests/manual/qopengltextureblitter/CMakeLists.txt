# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qopengltextureblitter.pro.

#####################################################################
## qopengltextureblitter Binary:
#####################################################################

qt_internal_add_manual_test(qopengltextureblitter
    GUI
    SOURCES
        main.cpp
        qopengltextureblitwindow.cpp qopengltextureblitwindow.h
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL # special case
)

#### Keys ignored in scope 1:.:.:qopengltextureblitter.pro:<TRUE>:
# TEMPLATE = "app"
