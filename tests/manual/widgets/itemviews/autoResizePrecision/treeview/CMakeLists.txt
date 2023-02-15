# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testtree Binary:
#####################################################################

qt_internal_add_manual_test(testtree
    GUI
    SOURCES
        testtree.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
