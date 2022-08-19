# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from verifyexceptionthrown.pro.

#####################################################################
## verifyexceptionthrown Binary:
#####################################################################

qt_internal_add_executable(verifyexceptionthrown
    EXCEPTIONS
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_verifyexceptionthrown.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(verifyexceptionthrown)
# special case end
