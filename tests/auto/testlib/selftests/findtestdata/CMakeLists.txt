# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from findtestdata.pro.

#####################################################################
## findtestdata Binary:
#####################################################################

qt_internal_add_executable(findtestdata
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        findtestdata.cpp
    LIBRARIES
        Qt::Test
)

# Resources:
set(findtestdata_resource_files
    "qt.conf"
)

qt_internal_add_resource(findtestdata "findtestdata"
    PREFIX
        "/qt/etc/"
    FILES
        ${findtestdata_resource_files}
)


## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(findtestdata)
# special case end
