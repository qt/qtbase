# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qquaternion Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qquaternion
    SOURCES
        tst_qquaternion.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
)
