# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qcolumnview.pro.

#####################################################################
## tst_qcolumnview Test:
#####################################################################

qt_internal_add_test(tst_qcolumnview
    SOURCES
        ../../../../shared/fakedirmodel.h
        tst_qcolumnview.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
