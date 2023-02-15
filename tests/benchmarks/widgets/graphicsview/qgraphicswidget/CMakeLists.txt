# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qgraphicswidget Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qgraphicswidget
    SOURCES
        tst_qgraphicswidget.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
