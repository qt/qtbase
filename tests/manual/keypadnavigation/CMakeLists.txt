# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## keypadnavigation Binary:
#####################################################################

qt_internal_add_manual_test(keypadnavigation
    GUI
    SOURCES
        keypadnavigation.ui
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
