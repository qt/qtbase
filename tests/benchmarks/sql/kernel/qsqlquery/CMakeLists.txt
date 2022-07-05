# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qsqlquery.pro.

#####################################################################
## tst_bench_qsqlquery Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qsqlquery
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
        Qt::Test
)
