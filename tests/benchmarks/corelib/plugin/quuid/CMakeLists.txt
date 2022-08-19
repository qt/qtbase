# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_quuid Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_quuid
    SOURCES
        tst_bench_quuid.cpp
    LIBRARIES
        Qt::Test
)
