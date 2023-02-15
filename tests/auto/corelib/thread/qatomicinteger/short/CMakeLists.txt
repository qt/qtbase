# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qatomicinteger_short Test:
#####################################################################

qt_internal_add_test(tst_qatomicinteger_short
    SOURCES
        ../tst_qatomicinteger.cpp
    DEFINES
        QATOMIC_TEST_TYPE=short
        tst_QAtomicIntegerXX=tst_QAtomicInteger_short
)
