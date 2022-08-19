# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qlist Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qlist
    SOURCES
        tst_bench_qlist.cpp
    LIBRARIES
        Qt::Test
)
