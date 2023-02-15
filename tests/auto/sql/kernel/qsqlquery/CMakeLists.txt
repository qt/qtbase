# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlquery Test:
#####################################################################

qt_internal_add_test(tst_qsqlquery
    SOURCES
        tst_qsqlquery.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
