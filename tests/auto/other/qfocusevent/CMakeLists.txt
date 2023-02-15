# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfocusevent Test:
#####################################################################

qt_internal_add_test(tst_qfocusevent
    SOURCES
        tst_qfocusevent.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
