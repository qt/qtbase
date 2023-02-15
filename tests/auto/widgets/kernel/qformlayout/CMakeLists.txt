# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qformlayout Test:
#####################################################################

qt_internal_add_test(tst_qformlayout
    SOURCES
        tst_qformlayout.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
