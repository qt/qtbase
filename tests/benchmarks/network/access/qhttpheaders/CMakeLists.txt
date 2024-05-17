# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_benchmark(tst_bench_qhttpheaders
    SOURCES
        tst_bench_qhttpheaders.cpp
    LIBRARIES
        Qt::Test
        Qt::Network
)
