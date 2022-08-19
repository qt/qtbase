# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qfileinfo Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qfileinfo
    SOURCES
        tst_bench_qfileinfo.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
