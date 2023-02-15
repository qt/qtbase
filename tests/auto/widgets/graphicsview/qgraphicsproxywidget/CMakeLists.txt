# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicsproxywidget Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsproxywidget
    SOURCES
        tst_qgraphicsproxywidget.cpp
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
