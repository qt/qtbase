# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## flicker Binary:
#####################################################################

qt_internal_add_manual_test(flicker
    GUI
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
