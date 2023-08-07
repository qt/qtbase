# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Menurama Binary:
#####################################################################

qt_internal_add_manual_test(Menurama
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
        menuramaapplication.cpp menuramaapplication.h
    NO_PCH_SOURCES
        menuramaapplication.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
