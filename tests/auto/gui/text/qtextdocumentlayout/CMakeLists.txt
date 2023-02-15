# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
