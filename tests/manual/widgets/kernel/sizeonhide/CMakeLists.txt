# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## sizeonhide Binary:
#####################################################################

qt_internal_add_manual_test(sizeonhide
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Widgets
)
