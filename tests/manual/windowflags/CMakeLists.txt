# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from windowflags.pro.

#####################################################################
## tst_manual_windowflags Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_windowflags
    GUI
    SOURCES
        controllerwindow.cpp controllerwindow.h
        controls.cpp controls.h
        main.cpp
        previewwindow.cpp previewwindow.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
