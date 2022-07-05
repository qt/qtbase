# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_benchmark(tst_bench_qproperty
    SOURCES
       tst_bench_qproperty.cpp
       propertytester.h
    LIBRARIES
        Qt::Core
        Qt::Test
)
