# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## rubberband Binary:
#####################################################################

qt_internal_add_manual_test(rubberband
    GUI
    SOURCES
        rubberbandtest.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
