# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## scrollarea Binary:
#####################################################################

qt_internal_add_manual_test(scrollarea
    GUI
    SOURCES
        main.cpp
        mousepangesturerecognizer.cpp mousepangesturerecognizer.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
