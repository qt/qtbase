# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_tostring Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_tostring
    SOURCES
        tst_tostring.cpp
    LIBRARIES
        Qt::Test
)
