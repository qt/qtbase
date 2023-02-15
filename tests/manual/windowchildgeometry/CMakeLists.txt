# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## windowchildgeometry Binary:
#####################################################################

qt_internal_add_manual_test(windowchildgeometry
    GUI
    SOURCES
        ../windowflags/controls.cpp ../windowflags/controls.h
        controllerwidget.cpp controllerwidget.h
        main.cpp
    INCLUDE_DIRECTORIES
        ../windowflags
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
