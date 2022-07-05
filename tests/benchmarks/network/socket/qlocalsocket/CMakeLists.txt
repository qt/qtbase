# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_bench_qlocalsocket Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qlocalsocket
    SOURCES
        tst_qlocalsocket.cpp
    LIBRARIES
        Qt::Network
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qlocalsocket.pro:<TRUE>:
# TEMPLATE = "app"
