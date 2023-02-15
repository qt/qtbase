# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## wheelevent Binary:
#####################################################################

qt_internal_add_manual_test(wheelevent
    GUI
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
