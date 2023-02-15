# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
