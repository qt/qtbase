# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qheaderview Binary:
#####################################################################

qt_internal_add_manual_test(qheaderview
    GUI
    SOURCES
        qheaderviewtest1.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
