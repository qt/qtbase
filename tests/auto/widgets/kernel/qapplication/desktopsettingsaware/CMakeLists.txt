# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## desktopsettingsaware Binary:
#####################################################################

qt_internal_add_executable(desktopsettingsaware_helper
    SOURCES
        main.cpp
    OUTPUT_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/..
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################
