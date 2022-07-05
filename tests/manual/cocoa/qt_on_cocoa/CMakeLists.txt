# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qt_on_cocoa.pro.

#####################################################################
## qt_on_cocoa Binary:
#####################################################################

qt_internal_add_manual_test(qt_on_cocoa
    GUI
    SOURCES
        main.mm
        rasterwindow.cpp rasterwindow.h
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    LIBRARIES
        ${FWAppKit}
        Qt::Gui
        Qt::Quick
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qt_on_cocoa.pro:<TRUE>:
# TEMPLATE = "app"
