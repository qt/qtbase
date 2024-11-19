# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qchar Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qchar LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "data/NormalizationTest.txt")

foreach(test tst_qchar tst_qchar_restricted_ascii tst_qchar_no_cast_from_ascii)
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

qt_internal_extend_target(tst_qchar_no_cast_from_ascii
    DEFINES
        QT_NO_CAST_FROM_ASCII
        tst_QChar=tst_QChar_no_cast_from_ascii
)
