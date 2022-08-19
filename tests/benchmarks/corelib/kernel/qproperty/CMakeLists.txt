# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_benchmark(tst_bench_qproperty
    SOURCES
       tst_bench_qproperty.cpp
       propertytester.h
    LIBRARIES
        Qt::Core
        Qt::Test
)
