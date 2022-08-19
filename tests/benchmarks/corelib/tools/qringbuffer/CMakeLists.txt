# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qringbuffer Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qringbuffer
    SOURCES
        tst_bench_qringbuffer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
