# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## regular_widgets Binary:
#####################################################################

qt_internal_add_manual_test(regular_widgets
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
