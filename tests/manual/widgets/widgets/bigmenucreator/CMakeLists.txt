# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from bigmenucreator.pro.

#####################################################################
## BigMenuCreator Binary:
#####################################################################

qt_internal_add_manual_test(BigMenuCreator
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
    DEFINES
        QT_DEPRECATED_WARNINGS
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:bigmenucreator.pro:<TRUE>:
# TEMPLATE = "app"
