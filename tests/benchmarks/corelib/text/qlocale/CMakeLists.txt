# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qlocale Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qlocale
    SOURCES
        tst_bench_qlocale.cpp
    LIBRARIES
        Qt::Test
)
