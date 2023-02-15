# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testtable2 Binary:
#####################################################################

qt_internal_add_manual_test(testtable2
    GUI
    SOURCES
        testtable2.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
