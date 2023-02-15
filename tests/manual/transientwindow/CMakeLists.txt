# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## transientwindow Binary:
#####################################################################

qt_internal_add_manual_test(transientwindow
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
