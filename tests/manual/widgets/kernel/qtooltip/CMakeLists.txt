# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtooltip Binary:
#####################################################################

qt_internal_add_manual_test(qtooltip
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
