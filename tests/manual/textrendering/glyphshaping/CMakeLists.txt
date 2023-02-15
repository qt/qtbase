# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## glyphshaping Binary:
#####################################################################

qt_internal_add_manual_test(glyphshaping
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
