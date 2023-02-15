# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qpainter Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qpainter
    SOURCES
        tst_qpainter.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
        Qt::Widgets
        Qt::WidgetsPrivate
)
