# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
