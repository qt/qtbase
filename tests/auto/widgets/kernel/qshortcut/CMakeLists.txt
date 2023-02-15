# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qshortcut Test:
#####################################################################

qt_internal_add_test(tst_qshortcut
    SOURCES
        tst_qshortcut.cpp
    INCLUDE_DIRECTORIES
        ..
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
