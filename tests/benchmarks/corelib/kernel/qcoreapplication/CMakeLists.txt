# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qcoreapplication Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qcoreapplication
    SOURCES
        tst_bench_qcoreapplication.cpp
    LIBRARIES
        Qt::Test
)
