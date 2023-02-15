# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tableview Binary:
#####################################################################

qt_internal_add_manual_test(tableview
    GUI
    SOURCES
        ../shared/shared.h
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
