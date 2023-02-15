# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlquerymodel Test:
#####################################################################

qt_internal_add_test(tst_qsqlquerymodel
    SOURCES
        tst_qsqlquerymodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Sql
        Qt::SqlPrivate
        Qt::Widgets
)
