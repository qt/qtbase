# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qfuture Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qfuture
    EXCEPTIONS
    SOURCES
        tst_bench_qfuture.cpp
    LIBRARIES
        Qt::Test
)
