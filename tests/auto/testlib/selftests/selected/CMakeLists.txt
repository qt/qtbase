# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## selected Binary:
#####################################################################

qt_internal_add_executable(selected
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_selected.cpp
    LIBRARIES
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(selected)
