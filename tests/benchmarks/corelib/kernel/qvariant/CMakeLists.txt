# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_bench_qvariant Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qvariant
    SOURCES
        tst_bench_qvariant.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_qvariant CONDITION NOT TARGET Qt::Gui
    LIBRARIES
        # Remove: gui
)
