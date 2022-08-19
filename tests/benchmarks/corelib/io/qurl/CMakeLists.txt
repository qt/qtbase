# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qurl Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qurl
    SOURCES
        tst_bench_qurl.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_qurl CONDITION WIN32
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)
