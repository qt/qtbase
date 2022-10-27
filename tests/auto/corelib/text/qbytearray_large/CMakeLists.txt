# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qbytearray_large
    SOURCES
        tst_qbytearray_large.cpp
    LIBRARIES
        Qt::Core
    TESTDATA "rfc3252.txt"
)

