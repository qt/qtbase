# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qt-client-widget Binary:
#####################################################################

qt_internal_add_manual_test(qt-client-widget
    GUI
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
