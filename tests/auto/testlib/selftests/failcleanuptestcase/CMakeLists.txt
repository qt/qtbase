# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
#####################################################################
## failcleanuptestcase Binary:
#####################################################################

qt_internal_add_executable(failcleanuptestcase
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_failcleanuptestcase.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(failcleanuptestcase)
