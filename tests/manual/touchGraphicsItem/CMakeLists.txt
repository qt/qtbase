# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## touchGraphicsItem Binary:
#####################################################################

qt_internal_add_manual_test(touchGraphicsItem
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
