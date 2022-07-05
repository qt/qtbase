# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qopengl.pro.

#####################################################################
## tst_qopengl Test:
#####################################################################

qt_internal_add_test(tst_qopengl
    SOURCES
        tst_qopengl.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qopengl CONDITION LINUX AND QT_FEATURE_xcb AND QT_FEATURE_xcb_glx_plugin
    DEFINES
        USE_GLX
)
