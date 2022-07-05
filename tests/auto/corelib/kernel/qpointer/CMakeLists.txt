# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qpointer.pro.

#####################################################################
## tst_qpointer Test:
#####################################################################

qt_internal_add_test(tst_qpointer
    SOURCES
        tst_qpointer.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpointer CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
