# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qchar Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/NormalizationTest.txt")

foreach(test tst_qchar tst_qchar_restricted_ascii)
    qt_internal_add_test(${test}
        NO_BATCH
        SOURCES
           tst_qchar.cpp
        LIBRARIES
            Qt::CorePrivate
        TESTDATA
            ${test_data}
    )
endforeach()

qt_internal_extend_target(tst_qchar_restricted_ascii
    DEFINES
        QT_RESTRICTED_CAST_FROM_ASCII
        tst_QChar=tst_QChar_restricted_ascii
)
