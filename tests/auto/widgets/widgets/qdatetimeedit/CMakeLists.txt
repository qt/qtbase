# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdatetimeedit Test:
#####################################################################

qt_internal_add_test(tst_qdatetimeedit
    SOURCES
        tst_qdatetimeedit.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
