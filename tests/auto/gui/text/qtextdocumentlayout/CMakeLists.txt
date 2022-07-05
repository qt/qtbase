# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qtextdocumentlayout.pro.

#####################################################################
## tst_qtextdocumentlayout Test:
#####################################################################

qt_internal_add_test(tst_qtextdocumentlayout
    SOURCES
        tst_qtextdocumentlayout.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtextdocumentlayout CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
