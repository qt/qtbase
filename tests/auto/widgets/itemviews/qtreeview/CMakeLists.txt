# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtreeview Test:
#####################################################################

qt_internal_add_test(tst_qtreeview
    SOURCES
        ../../../../shared/fakedirmodel.h
        tst_qtreeview.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
