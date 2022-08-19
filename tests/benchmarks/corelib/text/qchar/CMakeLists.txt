# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qchar Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qchar
    SOURCES
        tst_bench_qchar.cpp
    LIBRARIES
        Qt::Test
)
