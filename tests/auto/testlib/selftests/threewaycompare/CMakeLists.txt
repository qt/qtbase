# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_executable(threewaycompare
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_threewaycompare.cpp
    LIBRARIES
        Qt::Test
)

qt_internal_apply_testlib_coverage_options(threewaycompare)
