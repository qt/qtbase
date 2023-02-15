# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_macplist Test:
#####################################################################

qt_internal_add_test(tst_macplist
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_macplist.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################
