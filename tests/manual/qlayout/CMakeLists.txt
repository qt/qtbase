# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qlayout Binary:
#####################################################################

qt_internal_add_manual_test(qlayout
    GUI
    SOURCES
        gridwidget.cpp gridwidget.h
        hbwidget.cpp hbwidget.h
        main.cpp
        mainwindow.cpp mainwindow.h
        vbwidget.cpp vbwidget.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
