# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qlocalsocket Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qlocalsocket
    SOURCES
        tst_qlocalsocket.cpp
    LIBRARIES
        Qt::Network
        Qt::Test
)
