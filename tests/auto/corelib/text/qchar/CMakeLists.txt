# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qchar.pro.

#####################################################################
## tst_qchar Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/NormalizationTest.txt")

qt_internal_add_test(tst_qchar
    SOURCES
        tst_qchar.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)
