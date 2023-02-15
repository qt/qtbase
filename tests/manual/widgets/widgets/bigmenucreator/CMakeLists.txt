# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
