# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## delegate Binary:
#####################################################################

qt_internal_add_manual_test(delegate
    GUI
    SOURCES
        example.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
