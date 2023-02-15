# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlrelationaldelegate Test:
#####################################################################

qt_internal_add_test(tst_qsqlrelationaldelegate
    SOURCES
        tst_qsqlrelationaldelegate.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
        Qt::Widgets
)
