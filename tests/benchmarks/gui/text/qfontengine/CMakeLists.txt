# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_QFontEngine Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_QFontEngine
    SOURCES
        ../../../../auto/gui/text/qfontengine/tst_qfontengine.cpp
        tst_qfontengine.cpp
    DEFINES
        QFONTENGINE_BENCHMARK
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
)
