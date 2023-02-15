# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qpixmap Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qpixmap
    SOURCES
        tst_qpixmap.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
)
