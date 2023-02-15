# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## triangulator Binary:
#####################################################################

qt_internal_add_manual_test(triangulator
    GUI
    SOURCES
        main.cpp
        triviswidget.cpp triviswidget.h
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
