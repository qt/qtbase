# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## childwindowcontainer Binary:
#####################################################################

qt_internal_add_manual_test(childwindowcontainer
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
