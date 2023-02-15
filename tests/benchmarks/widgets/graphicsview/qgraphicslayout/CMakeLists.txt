# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qgraphicslayout Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qgraphicslayout
    SOURCES
        tst_qgraphicslayout.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)

# TEMPLATE = "app"
