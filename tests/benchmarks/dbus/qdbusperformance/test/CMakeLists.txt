# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusperformance Binary:
#####################################################################

qt_internal_add_benchmark(tst_qdbusperformance
    SOURCES
        ../serverobject.h
        ../tst_qdbusperformance.cpp
    LIBRARIES
        Qt::DBus
        Qt::Test
)
