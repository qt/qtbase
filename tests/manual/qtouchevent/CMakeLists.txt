# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qtouchevent.pro.

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
