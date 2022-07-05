# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from device_information.pro.

#####################################################################
## device_information Binary:
#####################################################################

qt_internal_add_manual_test(device_information
    GUI
    SOURCES
        main.cpp
        tabletwidget.cpp tabletwidget.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
