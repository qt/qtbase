# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtexttable Test:
#####################################################################

qt_internal_add_test(tst_qtexttable
    SOURCES
        tst_qtexttable.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtexttable CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
