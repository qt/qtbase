# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## paster Binary:
#####################################################################

set(args OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

qt_internal_add_executable(paster
    ${args}
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
)
