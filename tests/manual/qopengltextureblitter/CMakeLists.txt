# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
