# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qbytearray.pro.

#####################################################################
## tst_qbytearray Test:
#####################################################################

# Collect test data
list(APPEND test_data "rfc3252.txt")

qt_internal_add_test(tst_qbytearray
    SOURCES
        tst_qbytearray.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qbytearray CONDITION APPLE
    SOURCES
        tst_qbytearray_mac.mm
    LIBRARIES
        ${FWFoundation}
)
