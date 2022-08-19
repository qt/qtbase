# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qreadwritelock Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qreadwritelock
    SOURCES
        tst_bench_qreadwritelock.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
