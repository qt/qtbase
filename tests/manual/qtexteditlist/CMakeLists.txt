# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtexteditlist Binary:
#####################################################################

qt_internal_add_manual_test(qtexteditlist
    GUI
    SOURCES
        main.cpp
        widget.cpp widget.h widget.ui
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
