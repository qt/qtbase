# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
