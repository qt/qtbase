# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtimer_vs_qmetaobject Binary:
#####################################################################

qt_internal_add_benchmark(qtimer_vs_qmetaobject
    SOURCES
        tst_qtimer_vs_qmetaobject.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Test
)
