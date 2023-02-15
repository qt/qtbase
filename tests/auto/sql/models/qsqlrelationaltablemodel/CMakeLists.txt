# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlrelationaltablemodel Test:
#####################################################################

qt_internal_add_test(tst_qsqlrelationaltablemodel
    SOURCES
        tst_qsqlrelationaltablemodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
