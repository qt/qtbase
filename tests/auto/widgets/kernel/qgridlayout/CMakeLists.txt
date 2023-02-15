# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgridlayout Test:
#####################################################################

qt_internal_add_test(tst_qgridlayout
    SOURCES
        sortdialog.ui
        tst_qgridlayout.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    ENABLE_AUTOGEN_TOOLS
        uic
)
