# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qzip Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*
)

qt_internal_add_test(tst_qzip
    SOURCES
        tst_qzip.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)
