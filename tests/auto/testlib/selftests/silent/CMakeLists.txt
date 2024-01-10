# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## silent Binary:
#####################################################################

qt_internal_add_executable(silent
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_silent.cpp
    LIBRARIES
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(silent)
