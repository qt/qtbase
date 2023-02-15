# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtooltip Test:
#####################################################################

qt_internal_add_test(tst_qtooltip
    SOURCES
        tst_qtooltip.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
