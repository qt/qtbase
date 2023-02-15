# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qvector Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qvector
    SOURCES
        tst_bench_qvector.cpp
        outofline.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
