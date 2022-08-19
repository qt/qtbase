# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from pass.pro.

#####################################################################
## pass Binary:
#####################################################################

qt_internal_add_executable(pass
    NO_INSTALL # special case
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" # special case
    SOURCES
        tst_pass.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(pass)
# special case end

