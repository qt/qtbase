# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextboundaryfinder Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*
)

qt_internal_add_test(tst_qtextboundaryfinder
    SOURCES
        tst_qtextboundaryfinder.cpp
    TESTDATA ${test_data}
)
