# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qtabwidget.pro.

#####################################################################
## tst_qtabwidget Test:
#####################################################################

qt_internal_add_test(tst_qtabwidget
    SOURCES
        tst_qtabwidget.cpp
    INCLUDE_DIRECTORIES
        ..
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtabwidget CONDITION WIN32
    LIBRARIES
        user32
)
