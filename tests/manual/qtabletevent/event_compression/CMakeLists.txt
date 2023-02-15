# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## event_compression Binary:
#####################################################################

qt_internal_add_manual_test(event_compression
    SOURCES
        main.cpp
        mousestatwidget.cpp mousestatwidget.h
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
