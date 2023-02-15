# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qatomicinteger_char16_t Test:
#####################################################################

qt_internal_add_test(tst_qatomicinteger_char16_t
    SOURCES
        ../tst_qatomicinteger.cpp
    DEFINES
        QATOMIC_TEST_TYPE=char16_t
        tst_QAtomicIntegerXX=tst_QAtomicInteger_char16_t
)
