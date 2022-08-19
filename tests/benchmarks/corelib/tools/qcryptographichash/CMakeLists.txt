# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qcryptographichash Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qcryptographichash
    SOURCES
        tst_bench_qcryptographichash.cpp
    LIBRARIES
        Qt::Test
)
