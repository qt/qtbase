# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## screengadget Binary:
#####################################################################

qt_internal_add_manual_test(screengadget
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::GuiPrivate
)
