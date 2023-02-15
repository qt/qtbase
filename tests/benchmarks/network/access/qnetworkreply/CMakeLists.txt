# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qnetworkreply Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qnetworkreply
    SOURCES
        tst_qnetworkreply.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
        Qt::Test
)
