# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmainwindow Test:
#####################################################################

qt_internal_add_test(tst_qmainwindow
    SOURCES
        tst_qmainwindow.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
