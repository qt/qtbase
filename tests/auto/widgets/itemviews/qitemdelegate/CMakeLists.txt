# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qitemdelegate Test:
#####################################################################

qt_internal_add_test(tst_qitemdelegate
    SOURCES
        tst_qitemdelegate.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qitemdelegate CONDITION WIN32
    LIBRARIES
        user32
)
