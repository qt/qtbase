# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## touch Binary:
#####################################################################

qt_internal_add_manual_test(touch
    SOURCES
        gestures.cpp gestures.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
