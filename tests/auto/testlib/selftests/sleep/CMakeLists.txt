# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## sleep Binary:
#####################################################################

qt_internal_add_executable(sleep
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_sleep.cpp
    LIBRARIES
        Qt::Test
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(sleep)
