# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qsslsocket Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qsslsocket
    SOURCES
        tst_qsslsocket.cpp
    LIBRARIES
        Qt::Network
        Qt::Test
)
