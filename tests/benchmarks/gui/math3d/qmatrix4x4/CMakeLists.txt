# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qmatrix4x4 Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qmatrix4x4
    SOURCES
        tst_qmatrix4x4.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
)
