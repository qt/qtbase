# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtablewidget-sort-indicator Binary:
#####################################################################

qt_internal_add_manual_test(qtablewidget-sort-indicator
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Widgets
)
