# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qprogressbar Test:
#####################################################################

qt_internal_add_test(tst_qprogressbar
    SOURCES
        tst_qprogressbar.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
