# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qprocess Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qprocess
    SOURCES
        ../tst_bench_qprocess.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
