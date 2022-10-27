# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qvariant Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qvariant
    SOURCES
        tst_bench_qvariant.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_qvariant CONDITION TARGET Qt::Gui
    LIBRARIES
        Qt::Gui
)
