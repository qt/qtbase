# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qloggingregistry.pro.

#####################################################################
## tst_qloggingregistry Test:
#####################################################################

# Collect test data
list(APPEND test_data "qtlogging.ini")

qt_internal_add_test(tst_qloggingregistry
    SOURCES
        tst_qloggingregistry.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)
