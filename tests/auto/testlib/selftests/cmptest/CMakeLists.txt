# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## cmptest Binary:
#####################################################################

qt_internal_add_executable(cmptest
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_cmptest.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_extend_target(cmptest CONDITION TARGET Qt::Gui
    LIBRARIES
        Qt::Gui
)

qt_internal_apply_testlib_coverage_options(cmptest)
