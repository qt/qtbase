# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicsanchorlayout1 Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsanchorlayout1
    SOURCES
        tst_qgraphicsanchorlayout1.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
