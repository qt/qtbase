# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_benchmark(tst_bench_shared_ptr
    SOURCES
        tst_bench_shared_ptr.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Core
        Qt::Test
)
