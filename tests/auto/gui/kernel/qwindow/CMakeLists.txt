# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qwindow.pro.

#####################################################################
## tst_qwindow Test:
#####################################################################

qt_internal_add_test(tst_qwindow
    SOURCES
        tst_qwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qwindow CONDITION QT_FEATURE_dynamicgl AND WIN32
    LIBRARIES
        user32
)
