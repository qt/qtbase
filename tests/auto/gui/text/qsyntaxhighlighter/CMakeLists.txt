# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qsyntaxhighlighter.pro.

#####################################################################
## tst_qsyntaxhighlighter Test:
#####################################################################

qt_internal_add_test(tst_qsyntaxhighlighter
    SOURCES
        tst_qsyntaxhighlighter.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsyntaxhighlighter CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
