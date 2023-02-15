# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
