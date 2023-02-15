# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
