# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qitemmodel Test:
#####################################################################

qt_internal_add_test(tst_qitemmodel
    SOURCES
        tst_qitemmodel.cpp
    LIBRARIES
        Qt::Gui
        Qt::Sql
        Qt::Widgets
)
