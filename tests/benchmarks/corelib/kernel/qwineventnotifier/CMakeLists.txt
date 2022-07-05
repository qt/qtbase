# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_bench_qwineventnotifier Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qwineventnotifier
    SOURCES
        tst_bench_qwineventnotifier.cpp
    LIBRARIES
        Qt::Test
)
