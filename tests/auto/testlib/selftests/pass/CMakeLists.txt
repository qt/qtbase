# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## pass Binary:
#####################################################################

qt_internal_add_executable(pass
    NO_INSTALL
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    SOURCES
        tst_pass.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(pass)

