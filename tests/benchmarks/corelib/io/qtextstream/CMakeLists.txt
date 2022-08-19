# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qtextstream Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qtextstream
    SOURCES
        tst_bench_qtextstream.cpp
    LIBRARIES
        Qt::Test
)
