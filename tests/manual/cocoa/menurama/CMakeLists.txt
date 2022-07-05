# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from menurama.pro.

#####################################################################
## Menurama Binary:
#####################################################################

qt_internal_add_manual_test(Menurama
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
        menuramaapplication.cpp menuramaapplication.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:menurama.pro:<TRUE>:
# TEMPLATE = "app"
