# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qgraphicsitemgroup.pro.

#####################################################################
## qgraphicsitemgroup Binary:
#####################################################################

qt_internal_add_manual_test(qgraphicsitemgroup
    GUI
    SOURCES
        customitem.cpp customitem.h
        main.cpp
        widget.cpp widget.h widget.ui
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:qgraphicsitemgroup.pro:<TRUE>:
# TEMPLATE = "app"
