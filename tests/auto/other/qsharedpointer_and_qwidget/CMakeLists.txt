# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsharedpointer_and_qwidget Test:
#####################################################################

qt_internal_add_test(tst_qsharedpointer_and_qwidget
    SOURCES
        tst_qsharedpointer_and_qwidget.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
