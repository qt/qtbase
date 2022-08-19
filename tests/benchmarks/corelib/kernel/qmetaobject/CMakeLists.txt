# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qmetaobject Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qmetaobject
    SOURCES
        tst_bench_qmetaobject.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
