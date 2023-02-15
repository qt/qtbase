# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## pairdiagnostics Binary:
#####################################################################

qt_internal_add_executable(pairdiagnostics
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_pairdiagnostics.cpp
    LIBRARIES
        Qt::Test
)

qt_internal_apply_testlib_coverage_options(pairdiagnostics)
