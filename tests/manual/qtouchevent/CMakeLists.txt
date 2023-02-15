# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtouchevent Binary:
#####################################################################

qt_internal_add_manual_test(qtouchevent
    SOURCES
        form.ui
        main.cpp
        touchwidget.cpp touchwidget.h
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
