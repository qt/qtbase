# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractbutton Test:
#####################################################################

qt_internal_add_test(tst_qabstractbutton
    SOURCES
        tst_qabstractbutton.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
