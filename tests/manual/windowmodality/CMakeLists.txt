# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from windowmodality.pro.

#####################################################################
## windowmodality Binary:
#####################################################################

qt_internal_add_manual_test(windowmodality
    GUI
    SOURCES
        dialog.ui
        main.cpp
        widget.ui
    LIBRARIES
        Qt::Gui
        Qt::PrintSupport
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
