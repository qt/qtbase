# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdockwidget Test:
#####################################################################

qt_internal_add_test(tst_qdockwidget
    SOURCES
        tst_qdockwidget.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
