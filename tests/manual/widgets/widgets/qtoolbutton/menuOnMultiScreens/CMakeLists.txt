# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## menuOnMultiScreens Binary:
#####################################################################

qt_internal_add_manual_test(menuOnMultiScreens
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
