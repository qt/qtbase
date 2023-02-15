# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qbuttongroup Test:
#####################################################################

qt_internal_add_test(tst_qbuttongroup
    SOURCES
        tst_qbuttongroup.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
