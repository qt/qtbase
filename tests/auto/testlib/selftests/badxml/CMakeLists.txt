# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## badxml Binary:
#####################################################################

qt_internal_add_executable(badxml
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_badxml.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(badxml)
