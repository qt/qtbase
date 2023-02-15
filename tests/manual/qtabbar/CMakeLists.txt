# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtabbar Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_qtabbar
    GUI
    SOURCES
        main.cpp
        tabbarform.cpp tabbarform.h tabbarform.ui
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
