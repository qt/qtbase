# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from test.pro.

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
