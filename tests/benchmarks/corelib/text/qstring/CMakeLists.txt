# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qstring Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qstring
    SOURCES
        tst_bench_qstring.cpp
    LIBRARIES
        Qt::Test
)
