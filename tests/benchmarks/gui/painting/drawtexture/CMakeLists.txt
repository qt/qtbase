# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_drawtexture Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_drawtexture
    SOURCES
        tst_drawtexture.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
)
