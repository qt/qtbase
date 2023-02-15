# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tableview-span-navigation Binary:
#####################################################################

qt_internal_add_manual_test(tableview-span-navigation
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
