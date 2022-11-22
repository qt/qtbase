# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qutf8stringview Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qutf8stringview
    SOURCES
        tst_bench_qutf8stringview.cpp
    LIBRARIES
        Qt::Test
        Qt::CorePrivate
)
