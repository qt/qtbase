# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qsavefile.pro.

#####################################################################
## tst_qsavefile Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qsavefile.cpp")

qt_internal_add_test(tst_qsavefile
    SOURCES
        tst_qsavefile.cpp
    TESTDATA ${test_data}
)
