# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qplaintextedit.pro.

#####################################################################
## tst_qplaintextedit Test:
#####################################################################

qt_internal_add_test(tst_qplaintextedit
    SOURCES
        tst_qplaintextedit.cpp
    INCLUDE_DIRECTORIES
        ..
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qplaintextedit CONDITION MACOS
    LIBRARIES
        ${FWAppKit}
)
