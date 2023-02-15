# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testtable1 Binary:
#####################################################################

qt_internal_add_manual_test(testtable1
    GUI
    SOURCES
        testtable1.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
