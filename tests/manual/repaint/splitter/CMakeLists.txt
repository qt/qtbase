# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## splitter Binary:
#####################################################################

qt_internal_add_manual_test(splitter
    GUI
    SOURCES
        ../shared/shared.h
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
