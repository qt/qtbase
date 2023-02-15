# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## signaldumper Binary:
#####################################################################

qt_internal_add_executable(signaldumper
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_signaldumper.cpp
    LIBRARIES
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(signaldumper)
