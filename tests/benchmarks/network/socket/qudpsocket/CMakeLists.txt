# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qudpsocket Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qudpsocket
    SOURCES
        tst_qudpsocket.cpp
    LIBRARIES
        Qt::Network
        Qt::Test
)
