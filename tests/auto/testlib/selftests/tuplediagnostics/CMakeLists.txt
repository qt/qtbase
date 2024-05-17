# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tuplediagnostics Binary:
#####################################################################

qt_internal_add_executable(tuplediagnostics
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_tuplediagnostics.cpp
    LIBRARIES
        Qt::Test
)

qt_internal_apply_testlib_coverage_options(tuplediagnostics)
