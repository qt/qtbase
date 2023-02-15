# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_toplevel Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_toplevel
    SOURCES
        ../shared/shared.h
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
