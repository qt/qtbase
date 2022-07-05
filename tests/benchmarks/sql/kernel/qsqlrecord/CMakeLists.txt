# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qsqlrecord.pro.

#####################################################################
## tst_bench_qsqlrecord Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qsqlrecord
    SOURCES
        tst_bench_qsqlrecord.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
        Qt::Test
)
