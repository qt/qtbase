# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcryptographichash Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qcryptographichash
    SOURCES
        tst_qcryptographichash.cpp
    TESTDATA ${test_data}
)

if(QT_FEATURE_sanitize_address)
    set_property(TEST tst_qcryptographichash APPEND PROPERTY ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=900000")
endif()
