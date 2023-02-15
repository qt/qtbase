# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qtbench Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qtbench
    SOURCES
        tst_qtbench.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
