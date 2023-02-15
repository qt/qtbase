# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstackedlayout Test:
#####################################################################

qt_internal_add_test(tst_qstackedlayout
    SOURCES
        tst_qstackedlayout.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
