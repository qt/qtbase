# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
