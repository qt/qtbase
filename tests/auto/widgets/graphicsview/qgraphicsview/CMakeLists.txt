# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicsview Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsview
    SOURCES
        tst_qgraphicsview.cpp tst_qgraphicsview.h
        tst_qgraphicsview_2.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qgraphicsview CONDITION QT_FEATURE_opengl
    LIBRARIES
        Qt::OpenGL
        Qt::OpenGLWidgets
)
