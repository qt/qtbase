# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_benchmark(tst_bench_qlibraryinfo
    SOURCES
        tst_bench_qlibraryinfo.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)

qt_internal_add_resource(tst_bench_qlibraryinfo "qtconffiles"
    PREFIX "/"
    FILES
        absolute.qt.conf
        relative.qt.conf
        platforms.qt.conf
)
