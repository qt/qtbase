# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## verbose2 Binary:
#####################################################################

qt_internal_add_executable(verbose2
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        ../counting/tst_counting.cpp
    DEFINES
        TESTLIB_VERBOSITY_ARG=-v2
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(verbose2)
