# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_blendbench Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_blendbench
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
)
