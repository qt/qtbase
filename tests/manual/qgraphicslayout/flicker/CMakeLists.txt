# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from flicker.pro.

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
