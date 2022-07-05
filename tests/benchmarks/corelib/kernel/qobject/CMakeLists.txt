# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_bench_qobject Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qobject
    SOURCES
        tst_bench_qobject.cpp
        object.cpp object.h
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)
