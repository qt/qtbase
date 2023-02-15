# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsidebar Test:
#####################################################################

qt_internal_add_test(tst_qsidebar
    SOURCES
        tst_qsidebar.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
