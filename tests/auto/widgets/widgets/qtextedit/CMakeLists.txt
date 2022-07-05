# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qtextedit.pro.

#####################################################################
## tst_qtextedit Test:
#####################################################################

# Collect test data
list(APPEND test_data "fullWidthSelection")

qt_internal_add_test(tst_qtextedit
    SOURCES
        tst_qtextedit.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtextedit CONDITION MACOS
    LIBRARIES
        ${FWAppKit}
)
