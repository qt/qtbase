# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qt_on_cocoa Binary:
#####################################################################

qt_internal_add_manual_test(qt_on_cocoa
    GUI
    SOURCES
        main.mm
        rasterwindow.cpp rasterwindow.h
    LIBRARIES
        ${FWAppKit}
        Qt::Gui
        Qt::Quick
        Qt::Widgets
)
