# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicsanchorlayout Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsanchorlayout
    SOURCES
        tst_qgraphicsanchorlayout.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
